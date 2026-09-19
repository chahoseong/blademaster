#include "Combat/BlademasterGameplayAbility_Combo.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/BlademasterGameplayEffectContext.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayTag.h"
#include "Abilities/Tasks/AbilityTask_WaitInputPress.h"
#include "BlademasterGameplayTags.h"
#include "BlademasterLogChannels.h"
#include "Characters/BlademasterCharacter.h"
#include "Combat/BlademasterAttackDefinition.h"
#include "Combat/BlademasterComboDefinition.h"
#include "Combat/BlademasterWeaponTraceComponent.h"

#if !UE_BUILD_SHIPPING
#include "BlademasterDebug.h"
#endif

UBlademasterGameplayAbility_Combo::UBlademasterGameplayAbility_Combo()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	SetAssetTags(FGameplayTagContainer(BlademasterGameplayTags::Ability_Action_Attack));
	ActivationOwnedTags.AddTag(BlademasterGameplayTags::State_Attacking);
}

void UBlademasterGameplayAbility_Combo::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!ComboDefinition || ComboDefinition->Attacks.Num() == 0 || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	UE_LOG(LogBlademasterCombat, Verbose, TEXT("%s: 공격 시작 (%s, 총 %d타)"), *GetNameSafe(ActorInfo->AvatarActor.Get()), *GetNameSafe(ComboDefinition), ComboDefinition->Attacks.Num());

	if (ABlademasterCharacter* Character = Cast<ABlademasterCharacter>(ActorInfo->AvatarActor.Get()))
	{
		if (UBlademasterWeaponTraceComponent* WeaponTrace = Character->GetWeaponTraceComponent())
		{
			WeaponTrace->OnWeaponHit.AddUObject(this, &UBlademasterGameplayAbility_Combo::OnWeaponHit);
		}

#if !UE_BUILD_SHIPPING
		Character->OnDrawDebug.AddUObject(this, &UBlademasterGameplayAbility_Combo::DrawDebugText);
#endif
	}

	PlayAttack(0);
}

void UBlademasterGameplayAbility_Combo::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (ABlademasterCharacter* Character = ActorInfo ? Cast<ABlademasterCharacter>(ActorInfo->AvatarActor.Get()) : nullptr)
	{
		if (UBlademasterWeaponTraceComponent* WeaponTrace = Character->GetWeaponTraceComponent())
		{
			WeaponTrace->OnWeaponHit.RemoveAll(this);
		}

#if !UE_BUILD_SHIPPING
		Character->OnDrawDebug.RemoveAll(this);
#endif
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UBlademasterGameplayAbility_Combo::PlayAttack(int32 Index)
{
	UAnimMontage* Montage = ComboDefinition->Attacks.IsValidIndex(Index) ? ComboDefinition->Attacks[Index].Montage : nullptr;
	if (!Montage)
	{
		EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
		return;
	}

	CurrentAttackIndex = Index;
	bInputBuffered = false;
	bTransitioned = false;

	// 이전 타의 구간 태그가 아직 안 지워졌을 수 있다(이전 몽타주를 EndTask로 끊어도 실제
	// 블렌드아웃·NotifyEnd는 나중에 처리된다). 남아있으면 이번 타의 WaitGameplayTagAdded가
	// "이미 켜져 있으니 즉시 발동"으로 오작동해 조기 소모되므로, 여기서 미리 정리해둔다.
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->RemoveLooseGameplayTag(BlademasterGameplayTags::Attack_Window_Input);
		ASC->RemoveLooseGameplayTag(BlademasterGameplayTags::Attack_Window_Combo);
		ASC->RemoveLooseGameplayTag(BlademasterGameplayTags::Attack_Window_Cancel);
	}

	CurrentMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, Montage);
	CurrentMontageTask->OnCompleted.AddDynamic(this, &UBlademasterGameplayAbility_Combo::OnMontageCompleted);
	CurrentMontageTask->OnInterrupted.AddDynamic(this, &UBlademasterGameplayAbility_Combo::OnMontageInterrupted);
	CurrentMontageTask->OnCancelled.AddDynamic(this, &UBlademasterGameplayAbility_Combo::OnMontageInterrupted);
	CurrentMontageTask->ReadyForActivation();

	// 콤보 데이터의 마지막 타에서는 이어가기 구간이 있어도 무시한다 — 감지 태스크 자체를 안 건다.
	const bool bIsLastAttack = (Index == ComboDefinition->Attacks.Num() - 1);
	if (!bIsLastAttack)
	{
		ListenForInput();

		UAbilityTask_WaitGameplayTagAdded* ComboWindowTask = UAbilityTask_WaitGameplayTagAdded::WaitGameplayTagAdd(this, BlademasterGameplayTags::Attack_Window_Combo, nullptr, true);
		ComboWindowTask->Added.AddDynamic(this, &UBlademasterGameplayAbility_Combo::OnComboWindowBegin);
		ComboWindowTask->ReadyForActivation();
	}
}

void UBlademasterGameplayAbility_Combo::ListenForInput()
{
	if (CurrentInputTask)
	{
		CurrentInputTask->EndTask();
		CurrentInputTask = nullptr;
	}

	CurrentInputTask = UAbilityTask_WaitInputPress::WaitInputPress(this);
	CurrentInputTask->OnPress.AddDynamic(this, &UBlademasterGameplayAbility_Combo::OnInputPressed);
	CurrentInputTask->ReadyForActivation();
}

void UBlademasterGameplayAbility_Combo::OnMontageCompleted()
{
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UBlademasterGameplayAbility_Combo::OnMontageInterrupted()
{
	// 다음 타로 넘어가느라 일부러 중단시킨 경우 여기로 들어올 수 있다 — 그건 실패가 아니다.
	if (bTransitioned)
	{
		return;
	}

	UE_LOG(LogBlademasterCombat, Verbose, TEXT("%s: 공격 취소 (%d타)"), *GetNameSafe(GetAvatarActorFromActorInfo()), CurrentAttackIndex + 1);

	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, true);
}

void UBlademasterGameplayAbility_Combo::OnInputPressed(float TimeWaited)
{
	const UAbilitySystemComponent* AbilitySystemComponent = GetAbilitySystemComponentFromActorInfo();

	// 이어가기 구간 중 입력이면 기억할 필요 없이 바로 다음 타로 넘어간다.
	if (AbilitySystemComponent && AbilitySystemComponent->HasMatchingGameplayTag(BlademasterGameplayTags::Attack_Window_Combo))
	{
		UE_LOG(LogBlademasterCombat, Verbose, TEXT("%s: 타 전환 %d타 -> %d타 (늦은 입력)"), *GetNameSafe(GetAvatarActorFromActorInfo()), CurrentAttackIndex + 1, CurrentAttackIndex + 2);
		ProceedToNextAttack();
		return;
	}

	// 선입력 구간 밖의 입력(방향 전환 중 실수로 누름 등)은 그냥 버린다.
	if (AbilitySystemComponent && AbilitySystemComponent->HasMatchingGameplayTag(BlademasterGameplayTags::Attack_Window_Input))
	{
		bInputBuffered = true;
	}

	// WaitInputPress는 한 번 쓰면 끝나므로, 아직 다음 타로 넘어가지 않았다면 다시 걸어서 계속 듣는다.
	if (!bTransitioned)
	{
		ListenForInput();
	}
}

void UBlademasterGameplayAbility_Combo::OnComboWindowBegin()
{
	if (bInputBuffered)
	{
		UE_LOG(LogBlademasterCombat, Verbose, TEXT("%s: 타 전환 %d타 -> %d타 (선입력)"), *GetNameSafe(GetAvatarActorFromActorInfo()), CurrentAttackIndex + 1, CurrentAttackIndex + 2);
		ProceedToNextAttack();
	}
}

void UBlademasterGameplayAbility_Combo::OnWeaponHit(const FHitResult& Hit)
{
	AActor* HitActor = Hit.GetActor();
	UAbilitySystemComponent* AbilitySystemComponent = GetAbilitySystemComponentFromActorInfo();
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!HitActor || !AbilitySystemComponent || !Avatar || !ComboDefinition || !ComboDefinition->Attacks.IsValidIndex(CurrentAttackIndex))
	{
		return;
	}

	// 데미지 GameplayEffect 자체는 여기서 만들지 않는다 — 이 이벤트가 하는 일은 "공격자가 결정한
	// 공격 데이터를 들고 있으니, 맞은 대상의 피격 어빌리티가 알아서 처리하라"는 통지까지다.
	FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
	EffectContext.AddHitResult(Hit);

	if (FBlademasterGameplayEffectContext* BlademasterContext = static_cast<FBlademasterGameplayEffectContext*>(EffectContext.Get()))
	{
		const FBlademasterAttackDefinition& AttackDefinition = ComboDefinition->Attacks[CurrentAttackIndex];
		BlademasterContext->HealthDamage = AttackDefinition.HealthDamage;
		BlademasterContext->PostureDamage = AttackDefinition.PostureDamage;
	}

	FGameplayEventData Payload;
	Payload.EventTag = BlademasterGameplayTags::GameplayEvent_WeaponHit;
	Payload.Instigator = Avatar;
	Payload.Target = HitActor;
	Payload.ContextHandle = EffectContext;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(HitActor, Payload.EventTag, Payload);
}

void UBlademasterGameplayAbility_Combo::ProceedToNextAttack()
{
	if (bTransitioned)
	{
		return;
	}
	bTransitioned = true;

	if (CurrentMontageTask)
	{
		CurrentMontageTask->EndTask();
		CurrentMontageTask = nullptr;
	}

	if (CurrentInputTask)
	{
		CurrentInputTask->EndTask();
		CurrentInputTask = nullptr;
	}

	PlayAttack(CurrentAttackIndex + 1);
}

#if !UE_BUILD_SHIPPING
void UBlademasterGameplayAbility_Combo::DrawDebugText()
{
	const AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Avatar || !ComboDefinition)
	{
		return;
	}

	const FString Text = FString::Printf(TEXT("Combo: %d/%d%s"), CurrentAttackIndex + 1, ComboDefinition->Attacks.Num(), bInputBuffered ? TEXT(" [선입력 대기]") : TEXT(""));
	BlademasterDebug::DrawDebugTextLine(Avatar, 3, Text, FColor::Yellow);
}
#endif
