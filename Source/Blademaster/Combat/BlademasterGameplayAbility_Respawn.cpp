#include "Combat/BlademasterGameplayAbility_Respawn.h"

#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "AbilitySystemComponent.h"
#include "BlademasterGameplayTags.h"
#include "BlademasterLogChannels.h"
#include "Characters/BlademasterCharacter.h"
#include "GameplayEffect.h"

UBlademasterGameplayAbility_Respawn::UBlademasterGameplayAbility_Respawn()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag = BlademasterGameplayTags::State_Death_Dead;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::OwnedTagAdded;
	AbilityTriggers.Add(TriggerData);
}

void UBlademasterGameplayAbility_Respawn::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	UE_LOG(LogBlademasterCombat, Verbose, TEXT("%s: GA_Respawn 활성화 (RespawnDelay=%.2f)"), *GetNameSafe(ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr), RespawnDelay);

	if (RespawnDelay <= 0.f)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilityTask_WaitDelay* Task = UAbilityTask_WaitDelay::WaitDelay(this, RespawnDelay);
	Task->OnFinish.AddDynamic(this, &UBlademasterGameplayAbility_Respawn::OnRespawnDelayFinished);
	Task->ReadyForActivation();
}

void UBlademasterGameplayAbility_Respawn::OnRespawnDelayFinished()
{
	UE_LOG(LogBlademasterCombat, Verbose, TEXT("%s: 부활 처리 시작"), *GetNameSafe(GetAvatarActorFromActorInfo()));

	UAbilitySystemComponent* AbilitySystemComponent = GetAbilitySystemComponentFromActorInfo();
	ABlademasterCharacter* Character = Cast<ABlademasterCharacter>(GetAvatarActorFromActorInfo());
	const TSubclassOf<UGameplayEffect> InitializeAttributesEffect = Character ? Character->GetInitializeAttributesEffect() : nullptr;

	if (Character)
	{
		Character->SetCombatCollisionEnabled(true);
	}

	if (AbilitySystemComponent && InitializeAttributesEffect)
	{
		FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
		EffectContext.AddSourceObject(Character);
		const FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(InitializeAttributesEffect, 1.f, EffectContext);
		if (SpecHandle.IsValid())
		{
			AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
		}
	}

	// 사망 상태의 소유자인 GA_Death를 취소한다. 사망 태그와 사망 몽타주는 GA_Death가 끝날 때 정리되고,
	// 몽타주가 내려가면 로코모션으로 돌아온다. 활성화 시점에 발동하는 CancelAbilitiesWithTag 선언으로는
	// 사망 순간에 바로 취소되므로, 대기가 끝난 이 시점에 명령형으로 한 번 취소한다.
	if (AbilitySystemComponent)
	{
		const FGameplayTagContainer DeathTags(BlademasterGameplayTags::Ability_Reaction_Death);
		AbilitySystemComponent->CancelAbilities(&DeathTags, nullptr, this);
	}

	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}
