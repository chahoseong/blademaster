#include "Combat/BlademasterGameplayAbility_HitReact.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystem/BlademasterGameplayEffectContext.h"
#include "Animation/AnimMontage.h"
#include "BlademasterGameplayTags.h"
#include "Characters/BlademasterCharacter.h"

UBlademasterGameplayAbility_HitReact::UBlademasterGameplayAbility_HitReact()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// 재생 중에 다시 맞으면 처음부터 다시 재생한다.
	bRetriggerInstancedAbility = true;

	SetAssetTags(FGameplayTagContainer(BlademasterGameplayTags::Ability_Reaction_Hit));

	// 콤보뿐 아니라 앞으로 생길 공격류 어빌리티도 Ability.Action 계열 태그만 붙이면 자동 적용된다.
	CancelAbilitiesWithTag.AddTag(BlademasterGameplayTags::Ability_Action);
	BlockAbilitiesWithTag.AddTag(BlademasterGameplayTags::Ability_Action);
	ActivationBlockedTags.AddTag(BlademasterGameplayTags::State_Death);

	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag = BlademasterGameplayTags::GameplayEvent_Reaction_Hit;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);
}

void UBlademasterGameplayAbility_HitReact::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	ABlademasterCharacter* Character = ActorInfo ? Cast<ABlademasterCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	const FBlademasterGameplayEffectContext* Context = TriggerEventData ? FBlademasterGameplayEffectContext::FromHandle(TriggerEventData->ContextHandle) : nullptr;
	if (!Character || !Context)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const TArray<TObjectPtr<UAnimMontage>>& Candidates = Character->GetHitReactMontages(Context->HitDirection);
	UAnimMontage* ReactMontage = Candidates.Num() > 0 ? Candidates[FMath::RandHelper(Candidates.Num())] : nullptr;
	if (!ReactMontage)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	UAbilityTask_PlayMontageAndWait* Task = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, ReactMontage);
	Task->OnCompleted.AddDynamic(this, &UBlademasterGameplayAbility_HitReact::OnMontageEnded);
	Task->OnInterrupted.AddDynamic(this, &UBlademasterGameplayAbility_HitReact::OnMontageEnded);
	Task->OnCancelled.AddDynamic(this, &UBlademasterGameplayAbility_HitReact::OnMontageEnded);
	Task->ReadyForActivation();
}

void UBlademasterGameplayAbility_HitReact::OnMontageEnded()
{
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}
