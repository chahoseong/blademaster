#include "Combat/BlademasterGameplayAbility_Hit.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystem/BlademasterGameplayEffectContext.h"
#include "BlademasterGameplayTags.h"
#include "GameplayEffect.h"

UBlademasterGameplayAbility_Hit::UBlademasterGameplayAbility_Hit()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;

	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag = BlademasterGameplayTags::GameplayEvent_WeaponHit;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);
}

void UBlademasterGameplayAbility_Hit::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	// ① 막기·회피가 끼어들 자리 — 지금은 비워둔다(M2).

	UAbilitySystemComponent* AbilitySystemComponent = GetAbilitySystemComponentFromActorInfo();
	const IAbilitySystemInterface* AttackerInterface = TriggerEventData ? Cast<IAbilitySystemInterface>(TriggerEventData->Instigator) : nullptr;
	UAbilitySystemComponent* AttackerAbilitySystemComponent = AttackerInterface ? AttackerInterface->GetAbilitySystemComponent() : nullptr;

	if (!TriggerEventData || !DamageEffectClass || !AbilitySystemComponent || !AttackerAbilitySystemComponent)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

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

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
