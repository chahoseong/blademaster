#include "Combat/BlademasterGameplayAbility_GuardReact.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Animation/AnimMontage.h"
#include "BlademasterGameplayTags.h"
#include "Characters/BlademasterCharacter.h"

UBlademasterGameplayAbility_GuardReact::UBlademasterGameplayAbility_GuardReact()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// 재생 중에 다시 막으면 처음부터 다시 재생한다.
	bRetriggerInstancedAbility = true;

	SetAssetTags(FGameplayTagContainer(BlademasterGameplayTags::Ability_Reaction_Guard));

	// 가드(Ability.Action.Guard)는 끊지도 막지도 않는다 — 가드를 유지한 채 계속 막을 수 있어야 한다.
	BlockAbilitiesWithTag.AddTag(BlademasterGameplayTags::Ability_Action_Attack);
	ActivationBlockedTags.AddTag(BlademasterGameplayTags::State_Death);

	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag = BlademasterGameplayTags::GameplayEvent_Reaction_Guard;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);
}

void UBlademasterGameplayAbility_GuardReact::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	const ABlademasterCharacter* Character = ActorInfo ? Cast<ABlademasterCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	const TArray<TObjectPtr<UAnimMontage>>* Candidates = Character ? &Character->GetGuardReactMontages() : nullptr;
	UAnimMontage* ReactMontage = Candidates && Candidates->Num() > 0 ? (*Candidates)[FMath::RandHelper(Candidates->Num())] : nullptr;
	if (!ReactMontage)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	UAbilityTask_PlayMontageAndWait* Task = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, ReactMontage);
	Task->OnCompleted.AddDynamic(this, &UBlademasterGameplayAbility_GuardReact::OnMontageEnded);
	Task->OnInterrupted.AddDynamic(this, &UBlademasterGameplayAbility_GuardReact::OnMontageEnded);
	Task->OnCancelled.AddDynamic(this, &UBlademasterGameplayAbility_GuardReact::OnMontageEnded);
	Task->ReadyForActivation();
}

void UBlademasterGameplayAbility_GuardReact::OnMontageEnded()
{

	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}
