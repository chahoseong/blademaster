#include "AbilitySystem/BlademasterAttributeSet.h"

#include "BlademasterLogChannels.h"
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

void UBlademasterAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UBlademasterAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBlademasterAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBlademasterAttributeSet, Posture, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBlademasterAttributeSet, MaxPosture, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBlademasterAttributeSet, BasePostureRegenRate, COND_None, REPNOTIFY_Always);
}

void UBlademasterAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHealth());
	}
	else if (Attribute == GetPostureAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxPosture());
	}
}

void UBlademasterAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetIncomingHealthDamageAttribute())
	{
		const float Damage = GetIncomingHealthDamage();
		SetIncomingHealthDamage(0.f);

		if (Damage > 0.f)
		{
			const float OldHealth = GetHealth();
			const float NewHealth = FMath::Clamp(OldHealth - Damage, 0.f, GetMaxHealth());
			SetHealth(NewHealth);

			UE_LOG(LogBlademasterCombat, Log, TEXT("%s: 체력 %.1f 감소 (%.1f/%.1f 남음)"),
				*GetNameSafe(GetOwningActor()), OldHealth - NewHealth, NewHealth, GetMaxHealth());

			if (NewHealth <= 0.f)
			{
				OnOutOfHealth.Broadcast();
			}
		}
	}
	else if (Data.EvaluatedData.Attribute == GetIncomingPostureDamageAttribute())
	{
		const float Damage = GetIncomingPostureDamage();
		SetIncomingPostureDamage(0.f);

		if (Damage > 0.f)
		{
			const float OldPosture = GetPosture();
			const float NewPosture = FMath::Clamp(OldPosture - Damage, 0.f, GetMaxPosture());
			SetPosture(NewPosture);

			UE_LOG(LogBlademasterCombat, Log, TEXT("%s: 자세 %.1f 감소 (%.1f/%.1f 남음)"),
				*GetNameSafe(GetOwningActor()), OldPosture - NewPosture, NewPosture, GetMaxPosture());
		}
	}
}

void UBlademasterAttributeSet::OnRep_Health(const FGameplayAttributeData& OldHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBlademasterAttributeSet, Health, OldHealth);
}

void UBlademasterAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBlademasterAttributeSet, MaxHealth, OldMaxHealth);
}

void UBlademasterAttributeSet::OnRep_Posture(const FGameplayAttributeData& OldPosture)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBlademasterAttributeSet, Posture, OldPosture);
}

void UBlademasterAttributeSet::OnRep_MaxPosture(const FGameplayAttributeData& OldMaxPosture)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBlademasterAttributeSet, MaxPosture, OldMaxPosture);
}

void UBlademasterAttributeSet::OnRep_BasePostureRegenRate(const FGameplayAttributeData& OldBasePostureRegenRate)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBlademasterAttributeSet, BasePostureRegenRate, OldBasePostureRegenRate);
}
