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
	TriggerData.TriggerTag = BlademasterGameplayTags::State_Dead;
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

	if (AbilitySystemComponent)
	{
		// 사망 몽타주는 자동 블렌드 아웃을 꺼서 끝 자세를 유지하므로, 몽타주 자체가 "재생 중"
		// 상태로 계속 남아있다. 명시적으로 내려야 원래 자세(로코모션)로 돌아온다.
		AbilitySystemComponent->CurrentMontageStop(0.25f);
	}

	if (AbilitySystemComponent && InitializeAttributesEffect)
	{
		const FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
		const FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(InitializeAttributesEffect, 1.f, EffectContext);
		if (SpecHandle.IsValid())
		{
			AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
		}
	}

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->RemoveLooseGameplayTag(BlademasterGameplayTags::State_Dead);
	}

	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}
