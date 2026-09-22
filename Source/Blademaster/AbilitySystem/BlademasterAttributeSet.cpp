#include "AbilitySystem/BlademasterAttributeSet.h"

#include "BlademasterLogChannels.h"
#include "GameplayEffectExtension.h"

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

void UBlademasterAttributeSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);

	// 회복 GE는 자세에 직접 더하므로 기본값도 범위 안에 둔다. 현재값만 막으면 기본값이 최대치를 넘어 쌓인다.
	if (Attribute == GetPostureAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxPosture());
	}
}

void UBlademasterAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	// 자세에 직접 더하는 것은 회복 GE뿐이다(피해는 IncomingPostureDamage를 거친다). 초기화 GE는
	// 자세를 덮어쓰므로(Override) 회복으로 보지 않는다.
	// 회복량은 최대치까지 남은 만큼으로 잘려 오므로 0이면 이미 가득 차 있는 것이다.
	if (Data.EvaluatedData.Attribute == GetPostureAttribute())
	{
		if (Data.EvaluatedData.ModifierOp != EGameplayModOp::AddBase || Data.EvaluatedData.Magnitude <= 0.f)
		{
			return;
		}

		const float NewPosture = GetPosture();
		const float OldPosture = NewPosture - Data.EvaluatedData.Magnitude;

		UE_LOG(LogBlademasterCombat, Verbose, TEXT("%s: 자세 회복 %.2f (%.1f/%.1f)"),
			*GetNameSafe(GetOwningActor()), Data.EvaluatedData.Magnitude, NewPosture, GetMaxPosture());

		if (OldPosture < GetMaxPosture() && NewPosture >= GetMaxPosture())
		{
			UE_LOG(LogBlademasterCombat, Log, TEXT("%s: 자세가 최대치로 회복됐다 (%.1f)"), *GetNameSafe(GetOwningActor()), NewPosture);
		}
		return;
	}

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
