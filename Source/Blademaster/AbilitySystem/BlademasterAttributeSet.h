#pragma once

#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "CoreMinimal.h"
#include "BlademasterAttributeSet.generated.h"

UCLASS()
class BLADEMASTER_API UBlademasterAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	// 체력이 0이 되면 알린다. 반영 지점(PostGameplayEffectExecute) 한 곳에서만 브로드캐스트한다.
	DECLARE_MULTICAST_DELEGATE(FOnOutOfHealth);
	FOnOutOfHealth OnOutOfHealth;

	UPROPERTY(BlueprintReadOnly, Category = "Health")
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS_BASIC(UBlademasterAttributeSet, Health)

	UPROPERTY(BlueprintReadOnly, Category = "Health")
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS_BASIC(UBlademasterAttributeSet, MaxHealth)

	UPROPERTY(BlueprintReadOnly, Category = "Posture")
	FGameplayAttributeData Posture;
	ATTRIBUTE_ACCESSORS_BASIC(UBlademasterAttributeSet, Posture)

	UPROPERTY(BlueprintReadOnly, Category = "Posture")
	FGameplayAttributeData MaxPosture;
	ATTRIBUTE_ACCESSORS_BASIC(UBlademasterAttributeSet, MaxPosture)

	// 회복 속도 = Lerp(MinPostureRegenRate, BasePostureRegenRate, 체력 비율 ^ PostureRegenHealthExponent) — Specs/001-posture.md R-4.

	// 체력이 가득할 때의 회복 속도(초당 자세).
	UPROPERTY(BlueprintReadOnly, Category = "Posture")
	FGameplayAttributeData BasePostureRegenRate;
	ATTRIBUTE_ACCESSORS_BASIC(UBlademasterAttributeSet, BasePostureRegenRate)

	// 체력이 바닥일 때의 회복 속도(초당 자세). 0보다 크고 BasePostureRegenRate보다 작다.
	UPROPERTY(BlueprintReadOnly, Category = "Posture")
	FGameplayAttributeData MinPostureRegenRate;
	ATTRIBUTE_ACCESSORS_BASIC(UBlademasterAttributeSet, MinPostureRegenRate)

	// 체력 비율에 거는 지수. 클수록 체력 영향이 커진다. 0보다 크다.
	UPROPERTY(BlueprintReadOnly, Category = "Posture")
	FGameplayAttributeData PostureRegenHealthExponent;
	ATTRIBUTE_ACCESSORS_BASIC(UBlademasterAttributeSet, PostureRegenHealthExponent)

	// 자세가 깎인 뒤 다시 회복되기 시작할 때까지의 시간(초). 회복 대기 GE의 지속 시간이 된다.
	UPROPERTY(BlueprintReadOnly, Category = "Posture")
	FGameplayAttributeData PostureRegenDelay;
	ATTRIBUTE_ACCESSORS_BASIC(UBlademasterAttributeSet, PostureRegenDelay)

	// 붕괴가 지속되는 시간(초) — Specs/002-stagger.md R-2. 붕괴에 진입할 때 읽는다.
	UPROPERTY(BlueprintReadOnly, Category = "Posture")
	FGameplayAttributeData StaggerDuration;
	ATTRIBUTE_ACCESSORS_BASIC(UBlademasterAttributeSet, StaggerDuration)

	// 메타 어트리뷰트 — 데미지 GE가 여기 값을 넣으면 PostGameplayEffectExecute가 받아서 실제
	// 체력·자세에 반영하고 0으로 리셋한다.
	UPROPERTY(BlueprintReadOnly, Category = "Damage")
	FGameplayAttributeData IncomingHealthDamage;
	ATTRIBUTE_ACCESSORS_BASIC(UBlademasterAttributeSet, IncomingHealthDamage)

	UPROPERTY(BlueprintReadOnly, Category = "Damage")
	FGameplayAttributeData IncomingPostureDamage;
	ATTRIBUTE_ACCESSORS_BASIC(UBlademasterAttributeSet, IncomingPostureDamage)
};
