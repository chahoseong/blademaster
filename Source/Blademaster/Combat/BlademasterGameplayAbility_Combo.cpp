#include "Combat/BlademasterGameplayAbility_Combo.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "BlademasterGameplayTags.h"
#include "Combat/BlademasterAttackDefinition.h"
#include "Combat/BlademasterComboDefinition.h"

UBlademasterGameplayAbility_Combo::UBlademasterGameplayAbility_Combo()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	SetAssetTags(FGameplayTagContainer(TAG_Ability_Attack));
}

void UBlademasterGameplayAbility_Combo::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	UAnimMontage* Montage = (ComboDefinition && ComboDefinition->Attacks.Num() > 0) ? ComboDefinition->Attacks[0].Montage : nullptr;
	if (!Montage || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	UAbilityTask_PlayMontageAndWait* Task = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, Montage);
	Task->OnCompleted.AddDynamic(this, &UBlademasterGameplayAbility_Combo::OnMontageCompleted);
	Task->OnInterrupted.AddDynamic(this, &UBlademasterGameplayAbility_Combo::OnMontageInterrupted);
	Task->OnCancelled.AddDynamic(this, &UBlademasterGameplayAbility_Combo::OnMontageInterrupted);
	Task->ReadyForActivation();
}

void UBlademasterGameplayAbility_Combo::OnMontageCompleted()
{
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UBlademasterGameplayAbility_Combo::OnMontageInterrupted()
{
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, true);
}
