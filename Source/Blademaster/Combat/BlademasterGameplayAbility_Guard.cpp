#include "Combat/BlademasterGameplayAbility_Guard.h"

#include "Abilities/Tasks/AbilityTask_WaitInputRelease.h"
#include "BlademasterGameplayTags.h"
#include "BlademasterLogChannels.h"

UBlademasterGameplayAbility_Guard::UBlademasterGameplayAbility_Guard()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	SetAssetTags(FGameplayTagContainer(BlademasterGameplayTags::Ability_Action_Guard));
	ActivationOwnedTags.AddTag(BlademasterGameplayTags::State_Guard);

	BlockAbilitiesWithTag.AddTag(BlademasterGameplayTags::Ability_Action_Attack);
	ActivationBlockedTags.AddTag(BlademasterGameplayTags::State_Attacking);
}

void UBlademasterGameplayAbility_Guard::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	UE_LOG(LogBlademasterCombat, Log, TEXT("%s: 가드 시작"), *GetNameSafe(GetAvatarActorFromActorInfo()));

	// 이미 뗐으면 바로 끝난다(bTestAlreadyReleased). 입력 이벤트는 UBlademasterAbilitySystemComponent가 전달한다.
	UAbilityTask_WaitInputRelease* ReleaseTask = UAbilityTask_WaitInputRelease::WaitInputRelease(this, true);
	ReleaseTask->OnRelease.AddDynamic(this, &UBlademasterGameplayAbility_Guard::OnGuardInputReleased);
	ReleaseTask->ReadyForActivation();
}

void UBlademasterGameplayAbility_Guard::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	UE_LOG(LogBlademasterCombat, Log, TEXT("%s: 가드 %s"), *GetNameSafe(ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr),
		bWasCancelled ? TEXT("끊김") : TEXT("해제"));

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UBlademasterGameplayAbility_Guard::OnGuardInputReleased(float TimeHeld)
{
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}
