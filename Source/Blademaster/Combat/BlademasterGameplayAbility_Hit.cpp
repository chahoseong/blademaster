#include "Combat/BlademasterGameplayAbility_Hit.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "AbilitySystem/BlademasterAttributeSet.h"
#include "AbilitySystem/BlademasterGameplayEffectContext.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Animation/AnimMontage.h"
#include "BlademasterGameplayTags.h"
#include "BlademasterLogChannels.h"
#include "Characters/BlademasterCharacter.h"
#include "GameplayEffect.h"

UBlademasterGameplayAbility_Hit::UBlademasterGameplayAbility_Hit()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// 재생 중에 다시 맞으면 처음부터 다시 재생한다.
	bRetriggerInstancedAbility = true;

	SetAssetTags(FGameplayTagContainer(BlademasterGameplayTags::Ability_Reaction_Hit));

	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag = BlademasterGameplayTags::GameplayEvent_Weapon_Hit;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);
}

void UBlademasterGameplayAbility_Hit::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	// ① 막기·회피가 끼어들 자리 — 지금은 비워둔다(M2).

	bBlockedActionAbilities = false;

	UAbilitySystemComponent* AbilitySystemComponent = GetAbilitySystemComponentFromActorInfo();

	// 이미 죽어가는 중이거나 죽어있으면(사망 몽타주 재생 중 포함) 트리거 자체를 무시한다.
	// 판정(#12)은 죽으면 무기 채널 충돌을 꺼서 걸러내지만, 그 전에 날아온 이벤트에 대한 방어다.
	if (AbilitySystemComponent && (AbilitySystemComponent->HasMatchingGameplayTag(BlademasterGameplayTags::State_Death_Dying)
		|| AbilitySystemComponent->HasMatchingGameplayTag(BlademasterGameplayTags::State_Death_Dead)))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const IAbilitySystemInterface* AttackerInterface = TriggerEventData ? Cast<IAbilitySystemInterface>(TriggerEventData->Instigator) : nullptr;
	UAbilitySystemComponent* AttackerAbilitySystemComponent = AttackerInterface ? AttackerInterface->GetAbilitySystemComponent() : nullptr;
	ABlademasterCharacter* Victim = ActorInfo ? Cast<ABlademasterCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	const UBlademasterAttributeSet* AttributeSet = AbilitySystemComponent ? AbilitySystemComponent->GetSet<UBlademasterAttributeSet>() : nullptr;
	const FHitResult* Hit = TriggerEventData ? TriggerEventData->ContextHandle.GetHitResult() : nullptr;

	if (!TriggerEventData || !DamageEffectClass || !AbilitySystemComponent || !AttackerAbilitySystemComponent || !Victim || !AttributeSet || !Hit)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 하던 공격을 끊고, 이 어빌리티가 끝날 때까지(피격 반응 중) 새 공격을 막는다. 콤보뿐 아니라
	// 앞으로 생길 공격류 어빌리티도 자기 SetAssetTags에 Ability.Action 계열만 붙이면 자동 적용된다.
	const FGameplayTagContainer ActionTags(BlademasterGameplayTags::Ability_Action);
	AbilitySystemComponent->CancelAbilities(&ActionTags);
	AbilitySystemComponent->BlockAbilitiesWithTags(ActionTags);
	bBlockedActionAbilities = true;

	const float OldHealth = AttributeSet->GetHealth();
	const float OldPosture = AttributeSet->GetPosture();

	// ② 체력·자세를 깎는다. 스펙은 공격자의 ASC로 만든다 — 공격 데이터(데미지 값)는 판정(#12)이
	// 이벤트에 실어 보낸 컨텍스트에 이미 있으니 그대로 이어받는다.
	const FGameplayEffectSpecHandle SpecHandle = AttackerAbilitySystemComponent->MakeOutgoingSpec(DamageEffectClass, 1.f, TriggerEventData->ContextHandle);
	if (SpecHandle.IsValid())
	{
		if (const FBlademasterGameplayEffectContext* Context = static_cast<const FBlademasterGameplayEffectContext*>(TriggerEventData->ContextHandle.Get()))
		{
			SpecHandle.Data->SetSetByCallerMagnitude(BlademasterGameplayTags::SetByCaller_Damage_Health, Context->HealthDamage);
			SpecHandle.Data->SetSetByCallerMagnitude(BlademasterGameplayTags::SetByCaller_Damage_Posture, Context->PostureDamage);
		}

		AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}

	const float ActualHealthDamage = OldHealth - AttributeSet->GetHealth();
	const float ActualPostureDamage = OldPosture - AttributeSet->GetPosture();
	const bool bKilled = AttributeSet->GetHealth() <= 0.f;
	const EBlademasterHitDirection Direction = ComputeHitDirection(*Hit, Victim);

	// ④ "맞았다"를 알린다 — 한 곳에서 한 번.
	UE_LOG(LogBlademasterCombat, Log, TEXT("%s: 피격 처리 완료 (체력 %.1f / 자세 %.1f 감소%s)"),
		*GetNameSafe(Victim), ActualHealthDamage, ActualPostureDamage, bKilled ? TEXT(", 쓰러짐") : TEXT(""));

	if (bKilled)
	{
		AbilitySystemComponent->AddLooseGameplayTag(BlademasterGameplayTags::State_Death_Dying);

		// 무기 채널 충돌을 꺼서 판정(#12) 자체에서 빠지게 한다. 부활할 때(GA_Respawn) 다시 켠다.
		Victim->SetCombatCollisionEnabled(false);

		UAnimMontage* DeathMontage = Victim->GetDeathMontage(Direction);
		if (!DeathMontage)
		{
			UE_LOG(LogBlademasterCombat, Warning, TEXT("%s: 사망 몽타주가 없어 바로 State.Dead로 넘어간다"), *GetNameSafe(Victim));
			OnDeathMontageEnded();
			return;
		}

		// 어빌리티가 끝나도 몽타주가 끊기지 않게 한다 — 쓰러진 자세를 계속 유지해야 한다.
		// 자동 블렌드 아웃을 끈 몽타주라 "재생 끝남" 델리게이트 자체가 절대 안 온다 — 그래서
		// 완료 콜백을 기다리는 대신, 몽타주 길이만큼 직접 타이머를 걸어 State.Dead로 넘어간다.
		UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, NAME_None, DeathMontage, 1.f, NAME_None, /*bStopWhenAbilityEnds=*/ false);
		MontageTask->ReadyForActivation();

		const float DeathMontageLength = DeathMontage->GetPlayLength();
		UE_LOG(LogBlademasterCombat, Verbose, TEXT("%s: 사망 몽타주 %s 재생 시작 (%.2f초 뒤 State.Dead 전환 예정)"),
			*GetNameSafe(Victim), *GetNameSafe(DeathMontage), DeathMontageLength);

		UAbilityTask_WaitDelay* DelayTask = UAbilityTask_WaitDelay::WaitDelay(this, DeathMontageLength);
		DelayTask->OnFinish.AddDynamic(this, &UBlademasterGameplayAbility_Hit::OnDeathMontageEnded);
		DelayTask->ReadyForActivation();
		return;
	}

	const TArray<TObjectPtr<UAnimMontage>>& Candidates = Victim->GetHitReactMontages(Direction);
	UAnimMontage* ReactMontage = Candidates.Num() > 0 ? Candidates[FMath::RandHelper(Candidates.Num())] : nullptr;
	if (!ReactMontage)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	UAbilityTask_PlayMontageAndWait* Task = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, ReactMontage);
	Task->OnCompleted.AddDynamic(this, &UBlademasterGameplayAbility_Hit::OnHitReactMontageEnded);
	Task->OnInterrupted.AddDynamic(this, &UBlademasterGameplayAbility_Hit::OnHitReactMontageEnded);
	Task->OnCancelled.AddDynamic(this, &UBlademasterGameplayAbility_Hit::OnHitReactMontageEnded);
	Task->ReadyForActivation();
}

void UBlademasterGameplayAbility_Hit::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (bBlockedActionAbilities)
	{
		if (UAbilitySystemComponent* AbilitySystemComponent = GetAbilitySystemComponentFromActorInfo())
		{
			AbilitySystemComponent->UnBlockAbilitiesWithTags(FGameplayTagContainer(BlademasterGameplayTags::Ability_Action));
		}
		bBlockedActionAbilities = false;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UBlademasterGameplayAbility_Hit::OnHitReactMontageEnded()
{
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UBlademasterGameplayAbility_Hit::OnDeathMontageEnded()
{
	// State.Dying(사망 몽타주 재생 중) -> State.Dead(부활 대기 중)로 전환한다. GA_Respawn이
	// State.Dead의 OwnedTagAdded로 트리거되므로, 이 전환 시점이 곧 부활 대기시간의 시작점이다.
	if (UAbilitySystemComponent* AbilitySystemComponent = GetAbilitySystemComponentFromActorInfo())
	{
		UE_LOG(LogBlademasterCombat, Verbose, TEXT("%s: State.Dying -> State.Dead 전환"), *GetNameSafe(GetAvatarActorFromActorInfo()));
		AbilitySystemComponent->RemoveLooseGameplayTag(BlademasterGameplayTags::State_Death_Dying);
		AbilitySystemComponent->AddLooseGameplayTag(BlademasterGameplayTags::State_Death_Dead);
	}
	else
	{
		UE_LOG(LogBlademasterCombat, Warning, TEXT("OnDeathMontageEnded: AbilitySystemComponent를 못 찾음"));
	}

	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

EBlademasterHitDirection UBlademasterGameplayAbility_Hit::ComputeHitDirection(const FHitResult& Hit, const AActor* Victim)
{
	const FVector SwingDirection = (Hit.TraceEnd - Hit.TraceStart).GetSafeNormal();
	if (!Victim || SwingDirection.IsNearlyZero())
	{
		return EBlademasterHitDirection::Front;
	}

	// 칼이 피격자의 오른쪽으로 움직이며 지나갔다는 건, 반대편(왼쪽)에서 걸어들어왔다는 뜻이다.
	constexpr float FrontDotThreshold = 0.3f;
	const float RightDot = FVector::DotProduct(SwingDirection, Victim->GetActorRightVector());
	if (FMath::Abs(RightDot) < FrontDotThreshold)
	{
		return EBlademasterHitDirection::Front;
	}

	return RightDot > 0.f ? EBlademasterHitDirection::Left : EBlademasterHitDirection::Right;
}
