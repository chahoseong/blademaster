#include "Combat/BlademasterCombatComponent.h"

#include "AbilitySystem/BlademasterAttributeSet.h"
#include "AbilitySystem/BlademasterGameplayEffectContext.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Abilities/GameplayAbility.h"
#include "BlademasterGameplayTags.h"
#include "BlademasterLogChannels.h"
#include "GameplayEffect.h"

UBlademasterCombatComponent::UBlademasterCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UBlademasterCombatComponent::GrantStartingAbilities()
{
	AActor* Owner = GetOwner();
	const IAbilitySystemInterface* AbilitySystemInterface = Cast<IAbilitySystemInterface>(Owner);
	UAbilitySystemComponent* AbilitySystemComponent = AbilitySystemInterface ? AbilitySystemInterface->GetAbilitySystemComponent() : nullptr;
	if (!AbilitySystemComponent)
	{
		return;
	}

	for (const FBlademasterCombatAbilityToGrant& Grant : StartingAbilities)
	{
		if (!Grant.AbilityClass)
		{
			continue;
		}

		FGameplayAbilitySpec Spec(Grant.AbilityClass, 1, INDEX_NONE, Owner);
		Spec.GetDynamicSpecSourceTags().AddTag(Grant.InputTag);
		AbilitySystemComponent->GiveAbility(Spec);
	}
}

void UBlademasterCombatComponent::StartListeningForHits()
{
	const IAbilitySystemInterface* AbilitySystemInterface = Cast<IAbilitySystemInterface>(GetOwner());
	UAbilitySystemComponent* AbilitySystemComponent = AbilitySystemInterface ? AbilitySystemInterface->GetAbilitySystemComponent() : nullptr;
	if (!AbilitySystemComponent || WeaponHitEventHandle.IsValid())
	{
		return;
	}

	WeaponHitEventHandle = AbilitySystemComponent->AddGameplayEventTagContainerDelegate(
		FGameplayTagContainer(BlademasterGameplayTags::GameplayEvent_Weapon_Hit),
		FGameplayEventTagMulticastDelegate::FDelegate::CreateUObject(this, &UBlademasterCombatComponent::OnWeaponHitEvent));
}

void UBlademasterCombatComponent::ApplyPostureRegen()
{
	const IAbilitySystemInterface* AbilitySystemInterface = Cast<IAbilitySystemInterface>(GetOwner());
	UAbilitySystemComponent* AbilitySystemComponent = AbilitySystemInterface ? AbilitySystemInterface->GetAbilitySystemComponent() : nullptr;
	if (!AbilitySystemComponent || PostureRegenDelayTagHandle.IsValid())
	{
		return;
	}

	if (!PostureRegenEffectClass)
	{
		UE_LOG(LogBlademasterCombat, Warning, TEXT("%s: PostureRegenEffectClass가 지정되지 않아 자세가 회복되지 않는다"), *GetNameSafe(GetOwner()));
		return;
	}

	FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
	EffectContext.AddSourceObject(GetOwner());
	const FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(PostureRegenEffectClass, 1.f, EffectContext);
	if (SpecHandle.IsValid())
	{
		AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}

	// 대기가 끝나 회복이 다시 시작되는 시점을 로그로 남긴다.
	PostureRegenDelayTagHandle = AbilitySystemComponent->RegisterGameplayTagEvent(BlademasterGameplayTags::State_Posture_RegenDelay, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &UBlademasterCombatComponent::OnPostureRegenDelayTagChanged);
}

void UBlademasterCombatComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	const IAbilitySystemInterface* AbilitySystemInterface = Cast<IAbilitySystemInterface>(GetOwner());
	UAbilitySystemComponent* AbilitySystemComponent = AbilitySystemInterface ? AbilitySystemInterface->GetAbilitySystemComponent() : nullptr;

	if (WeaponHitEventHandle.IsValid())
	{
		if (AbilitySystemComponent)
		{
			AbilitySystemComponent->RemoveGameplayEventTagContainerDelegate(
				FGameplayTagContainer(BlademasterGameplayTags::GameplayEvent_Weapon_Hit), WeaponHitEventHandle);
		}
		WeaponHitEventHandle.Reset();
	}

	if (PostureRegenDelayTagHandle.IsValid())
	{
		if (AbilitySystemComponent)
		{
			AbilitySystemComponent->RegisterGameplayTagEvent(BlademasterGameplayTags::State_Posture_RegenDelay, EGameplayTagEventType::NewOrRemoved)
				.Remove(PostureRegenDelayTagHandle);
		}
		PostureRegenDelayTagHandle.Reset();
	}

	Super::EndPlay(EndPlayReason);
}

void UBlademasterCombatComponent::StartPostureRegenDelay(UAbilitySystemComponent* AbilitySystemComponent)
{
	if (!PostureRegenDelayEffectClass)
	{
		UE_LOG(LogBlademasterCombat, Warning, TEXT("%s: PostureRegenDelayEffectClass가 지정되지 않아 자세가 깎여도 회복이 멈추지 않는다"), *GetNameSafe(GetOwner()));
		return;
	}

	const bool bWasDelaying = AbilitySystemComponent->HasMatchingGameplayTag(BlademasterGameplayTags::State_Posture_RegenDelay);

	FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
	EffectContext.AddSourceObject(GetOwner());
	const FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(PostureRegenDelayEffectClass, 1.f, EffectContext);
	if (!SpecHandle.IsValid())
	{
		return;
	}
	AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());

	const UBlademasterAttributeSet* AttributeSet = AbilitySystemComponent->GetSet<UBlademasterAttributeSet>();
	UE_LOG(LogBlademasterCombat, Log, TEXT("%s: 자세 회복 대기 %s (%.2f초)"), *GetNameSafe(GetOwner()),
		bWasDelaying ? TEXT("다시 시작") : TEXT("시작"), AttributeSet ? AttributeSet->GetPostureRegenDelay() : 0.f);
}

void UBlademasterCombatComponent::OnPostureRegenDelayTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (NewCount != 0)
	{
		return;
	}

	// 붕괴 중에는 회복 GE가 State.Stagger로도 멈춰 있어서 대기가 끝나도 회복하지 않는다.
	const IAbilitySystemInterface* AbilitySystemInterface = Cast<IAbilitySystemInterface>(GetOwner());
	const UAbilitySystemComponent* AbilitySystemComponent = AbilitySystemInterface ? AbilitySystemInterface->GetAbilitySystemComponent() : nullptr;
	if (AbilitySystemComponent && AbilitySystemComponent->HasMatchingGameplayTag(BlademasterGameplayTags::State_Stagger))
	{
		UE_LOG(LogBlademasterCombat, Log, TEXT("%s: 자세 회복 대기가 끝났지만 붕괴 중이라 회복하지 않는다"), *GetNameSafe(GetOwner()));
		return;
	}

	UE_LOG(LogBlademasterCombat, Log, TEXT("%s: 자세 회복 대기가 끝나 회복을 다시 시작한다"), *GetNameSafe(GetOwner()));
}

void UBlademasterCombatComponent::OnWeaponHitEvent(FGameplayTag EventTag, const FGameplayEventData* Payload)
{
	const IAbilitySystemInterface* VictimInterface = Cast<IAbilitySystemInterface>(GetOwner());
	UAbilitySystemComponent* VictimAbilitySystemComponent = VictimInterface ? VictimInterface->GetAbilitySystemComponent() : nullptr;
	if (!Payload || !VictimAbilitySystemComponent)
	{
		return;
	}

	// 이미 죽었거나 죽어가는 중이면 무시한다. 판정은 죽으면 무기 채널 충돌을 꺼서 걸러내지만,
	// 그 전에 날아온 이벤트에 대한 방어다.
	if (VictimAbilitySystemComponent->HasMatchingGameplayTag(BlademasterGameplayTags::State_Death))
	{
		return;
	}

	// ① 막기·회피가 끼어들 자리 — 지금은 비워둔다(M2).

	const IAbilitySystemInterface* AttackerInterface = Cast<IAbilitySystemInterface>(Payload->Instigator.Get());
	UAbilitySystemComponent* AttackerAbilitySystemComponent = AttackerInterface ? AttackerInterface->GetAbilitySystemComponent() : nullptr;
	const UBlademasterAttributeSet* AttributeSet = VictimAbilitySystemComponent->GetSet<UBlademasterAttributeSet>();
	FGameplayEffectContextHandle ContextHandle = Payload->ContextHandle;
	FBlademasterGameplayEffectContext* Context = FBlademasterGameplayEffectContext::FromHandle(ContextHandle);
	const FHitResult* Hit = ContextHandle.GetHitResult();

	if (!AttackerAbilitySystemComponent || !AttributeSet || !Context || !Hit)
	{
		return;
	}

	if (!DamageEffectClass)
	{
		UE_LOG(LogBlademasterCombat, Warning, TEXT("%s: DamageEffectClass가 지정되지 않아 타격을 처리하지 못한다"), *GetNameSafe(GetOwner()));
		return;
	}

	// 피격 방향은 반응 어빌리티가 몽타주를 고를 때 읽도록 컨텍스트에 기록한다. 파생 이벤트도 이 컨텍스트를 그대로 쓴다.
	const EBlademasterHitDirection Direction = ComputeHitDirection(*Hit, GetOwner()->GetActorRightVector());
	Context->HitDirection = Direction;

	const float OldHealth = AttributeSet->GetHealth();
	const float OldPosture = AttributeSet->GetPosture();

	// 붕괴 중인지는 피해를 적용하기 전에 본다. 붕괴 중이면 데미지 GE의 자세 모디파이어가 적용되지 않는다(GE 데이터의 대상 태그 조건).
	const bool bWasStaggered = VictimAbilitySystemComponent->HasMatchingGameplayTag(BlademasterGameplayTags::State_Stagger);

	// ② 체력·자세를 깎는다. 스펙은 공격자의 ASC로 만든다 — 공격 데이터(데미지 값)는 공격자가
	// 이벤트에 실어 보낸 컨텍스트에 이미 있다. 인스턴트 GE는 동기적으로 실행되므로 적용 직후 값이 확정된다.
	const FGameplayEffectSpecHandle SpecHandle = AttackerAbilitySystemComponent->MakeOutgoingSpec(DamageEffectClass, 1.f, ContextHandle);
	if (!SpecHandle.IsValid())
	{
		return;
	}

	SpecHandle.Data->SetSetByCallerMagnitude(BlademasterGameplayTags::SetByCaller_Damage_Health, Context->HealthDamage);
	SpecHandle.Data->SetSetByCallerMagnitude(BlademasterGameplayTags::SetByCaller_Damage_Posture, Context->PostureDamage);
	VictimAbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());

	// 자세가 실제로 줄었을 때만 회복을 멈춘다(001 R-3). 피해가 0이었거나 이미 0이었으면 대기를 건드리지 않는다.
	if (AttributeSet->GetPosture() < OldPosture)
	{
		StartPostureRegenDelay(VictimAbilitySystemComponent);
	}

	// ③ 확정된 체력·자세로 결과를 하나 고르고, 그 결과를 이름으로 지정한 이벤트로 알린다.
	// 사망 > 붕괴 중(반응 없음) > 붕괴 진입 > 피격 — Specs/002-stagger.md R-1, R-4.
	// 한 GE 안에서 체력·자세가 함께 바뀌므로 둘 다 확정된 뒤에 고른다. 그래야 한 타에 사망과 붕괴가 함께 발동하지 않는다.
	const bool bKilled = AttributeSet->GetHealth() <= 0.f;
	const bool bEntersStagger = !bKilled && !bWasStaggered && AttributeSet->GetPosture() <= 0.f;

	FGameplayTag ReactionTag;
	const TCHAR* ResultName = TEXT("피격");
	if (bKilled)
	{
		ReactionTag = BlademasterGameplayTags::GameplayEvent_Reaction_Death;
		ResultName = TEXT("사망");
	}
	else if (bWasStaggered)
	{
		ResultName = TEXT("붕괴 중 — 반응 없음");
	}
	else if (bEntersStagger)
	{
		ReactionTag = BlademasterGameplayTags::GameplayEvent_Reaction_Stagger;
		ResultName = TEXT("붕괴 진입");
	}
	else
	{
		ReactionTag = BlademasterGameplayTags::GameplayEvent_Reaction_Hit;
	}

	UE_LOG(LogBlademasterCombat, Log, TEXT("%s: 타격 해석 — 방향 %s, 결과 %s (체력 %.1f / 자세 %.1f 감소)"),
		*GetNameSafe(GetOwner()), *UEnum::GetValueAsString(Direction), ResultName,
		OldHealth - AttributeSet->GetHealth(), OldPosture - AttributeSet->GetPosture());

	// 붕괴 중에 맞으면 피격 반응을 재생하지 않는다. 체력은 이미 깎였다.
	if (!ReactionTag.IsValid())
	{
		return;
	}

	FGameplayEventData ReactionPayload = *Payload;
	ReactionPayload.EventTag = ReactionTag;
	if (VictimAbilitySystemComponent->HandleGameplayEvent(ReactionTag, &ReactionPayload) == 0)
	{
		UE_LOG(LogBlademasterCombat, Warning, TEXT("%s: %s를 받아 활성화된 어빌리티가 없다. StartingAbilities를 확인할 것"),
			*GetNameSafe(GetOwner()), *ReactionTag.ToString());
	}
}

EBlademasterHitDirection UBlademasterCombatComponent::ComputeHitDirection(const FHitResult& Hit, const FVector& VictimRight)
{
	const FVector SwingDirection = (Hit.TraceEnd - Hit.TraceStart).GetSafeNormal();
	if (SwingDirection.IsNearlyZero())
	{
		return EBlademasterHitDirection::Front;
	}

	constexpr float FrontDotThreshold = 0.3f;
	const float RightDot = FVector::DotProduct(SwingDirection, VictimRight);
	if (FMath::Abs(RightDot) < FrontDotThreshold)
	{
		return EBlademasterHitDirection::Front;
	}

	return RightDot > 0.f ? EBlademasterHitDirection::Left : EBlademasterHitDirection::Right;
}

void UBlademasterCombatComponent::AbilityInputTagPressed(const FGameplayTag& InputTag)
{
	const IAbilitySystemInterface* AbilitySystemInterface = Cast<IAbilitySystemInterface>(GetOwner());
	UAbilitySystemComponent* AbilitySystemComponent = AbilitySystemInterface ? AbilitySystemInterface->GetAbilitySystemComponent() : nullptr;
	if (!AbilitySystemComponent || !InputTag.IsValid())
	{
		return;
	}

	for (FGameplayAbilitySpec& Spec : AbilitySystemComponent->GetActivatableAbilities())
	{
		if (!Spec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
		{
			continue;
		}

		AbilitySystemComponent->AbilitySpecInputPressed(Spec);
		if (!Spec.IsActive())
		{
			AbilitySystemComponent->TryActivateAbility(Spec.Handle);
		}
	}
}

void UBlademasterCombatComponent::AbilityInputTagReleased(const FGameplayTag& InputTag)
{
	const IAbilitySystemInterface* AbilitySystemInterface = Cast<IAbilitySystemInterface>(GetOwner());
	UAbilitySystemComponent* AbilitySystemComponent = AbilitySystemInterface ? AbilitySystemInterface->GetAbilitySystemComponent() : nullptr;
	if (!AbilitySystemComponent || !InputTag.IsValid())
	{
		return;
	}

	for (FGameplayAbilitySpec& Spec : AbilitySystemComponent->GetActivatableAbilities())
	{
		if (Spec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
		{
			AbilitySystemComponent->AbilitySpecInputReleased(Spec);
		}
	}
}
