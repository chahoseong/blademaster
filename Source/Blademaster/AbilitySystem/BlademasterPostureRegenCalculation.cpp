#include "AbilitySystem/BlademasterPostureRegenCalculation.h"

#include "AbilitySystem/BlademasterAttributeSet.h"

namespace
{
	FGameplayEffectAttributeCaptureDefinition CaptureTarget(const FGameplayAttribute& Attribute)
	{
		return FGameplayEffectAttributeCaptureDefinition(Attribute, EGameplayEffectAttributeCaptureSource::Target, false);
	}
}

UBlademasterPostureRegenCalculation::UBlademasterPostureRegenCalculation()
{
	HealthDef = CaptureTarget(UBlademasterAttributeSet::GetHealthAttribute());
	MaxHealthDef = CaptureTarget(UBlademasterAttributeSet::GetMaxHealthAttribute());
	PostureDef = CaptureTarget(UBlademasterAttributeSet::GetPostureAttribute());
	MaxPostureDef = CaptureTarget(UBlademasterAttributeSet::GetMaxPostureAttribute());
	BaseRateDef = CaptureTarget(UBlademasterAttributeSet::GetBasePostureRegenRateAttribute());
	MinRateDef = CaptureTarget(UBlademasterAttributeSet::GetMinPostureRegenRateAttribute());
	HealthExponentDef = CaptureTarget(UBlademasterAttributeSet::GetPostureRegenHealthExponentAttribute());

	RelevantAttributesToCapture.Add(HealthDef);
	RelevantAttributesToCapture.Add(MaxHealthDef);
	RelevantAttributesToCapture.Add(PostureDef);
	RelevantAttributesToCapture.Add(MaxPostureDef);
	RelevantAttributesToCapture.Add(BaseRateDef);
	RelevantAttributesToCapture.Add(MinRateDef);
	RelevantAttributesToCapture.Add(HealthExponentDef);
}

float UBlademasterPostureRegenCalculation::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	FAggregatorEvaluateParameters EvaluateParameters;
	EvaluateParameters.SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	EvaluateParameters.TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	auto Read = [&](const FGameplayEffectAttributeCaptureDefinition& Def)
	{
		float Value = 0.f;
		GetCapturedAttributeMagnitude(Def, Spec, EvaluateParameters, Value);
		return Value;
	};

	const float MaxHealth = Read(MaxHealthDef);
	const float HealthRatio = MaxHealth > 0.f ? FMath::Clamp(Read(HealthDef) / MaxHealth, 0.f, 1.f) : 0.f;
	const float Rate = FMath::Lerp(Read(MinRateDef), Read(BaseRateDef), FMath::Pow(HealthRatio, Read(HealthExponentDef)));

	const float Remaining = FMath::Max(0.f, Read(MaxPostureDef) - Read(PostureDef));
	return FMath::Min(Rate * Spec.GetPeriod(), Remaining);
}
