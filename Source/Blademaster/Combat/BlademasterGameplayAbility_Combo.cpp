#include "Combat/BlademasterGameplayAbility_Combo.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayTag.h"
#include "Abilities/Tasks/AbilityTask_WaitInputPress.h"
#include "BlademasterGameplayTags.h"
#include "Combat/BlademasterAttackDefinition.h"
#include "Combat/BlademasterComboDefinition.h"

UBlademasterGameplayAbility_Combo::UBlademasterGameplayAbility_Combo()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	SetAssetTags(FGameplayTagContainer(BlademasterGameplayTags::Ability_Attack));
	ActivationOwnedTags.AddTag(BlademasterGameplayTags::State_Attacking);
}

void UBlademasterGameplayAbility_Combo::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!ComboDefinition || ComboDefinition->Attacks.Num() == 0 || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	PlayAttack(0);
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

	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, true);
}

void UBlademasterGameplayAbility_Combo::OnInputPressed(float TimeWaited)
{
	const UAbilitySystemComponent* AbilitySystemComponent = GetAbilitySystemComponentFromActorInfo();

	// 이어가기 구간 중 입력이면 기억할 필요 없이 바로 다음 타로 넘어간다.
	if (AbilitySystemComponent && AbilitySystemComponent->HasMatchingGameplayTag(BlademasterGameplayTags::Attack_Window_Combo))
	{
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
		ProceedToNextAttack();
	}
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
