#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "BlademasterPostureRegenCalculation.generated.h"

// 자세 회복 GE(주기 GE)의 한 주기 회복량.
// 회복 속도 = Lerp(MinPostureRegenRate, BasePostureRegenRate, 체력 비율 ^ PostureRegenHealthExponent) — Specs/001-posture.md R-4.
// 대상의 어트리뷰트를 스냅샷하지 않고 캡처한다. 주기 GE는 실행할 때마다 크기를 다시 계산하므로
// 그 순간의 체력 비율이 반영된다. 최대치까지 남은 양을 넘지 않는다.
UCLASS()
class BLADEMASTER_API UBlademasterPostureRegenCalculation : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()

public:
	UBlademasterPostureRegenCalculation();

	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;

private:
	FGameplayEffectAttributeCaptureDefinition HealthDef;
	FGameplayEffectAttributeCaptureDefinition MaxHealthDef;
	FGameplayEffectAttributeCaptureDefinition PostureDef;
	FGameplayEffectAttributeCaptureDefinition MaxPostureDef;
	FGameplayEffectAttributeCaptureDefinition BaseRateDef;
	FGameplayEffectAttributeCaptureDefinition MinRateDef;
	FGameplayEffectAttributeCaptureDefinition HealthExponentDef;
};
