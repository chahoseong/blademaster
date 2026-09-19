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

	UPROPERTY(BlueprintReadOnly, Category = "Posture")
	FGameplayAttributeData BasePostureRegenRate;
	ATTRIBUTE_ACCESSORS_BASIC(UBlademasterAttributeSet, BasePostureRegenRate)

	// 메타 어트리뷰트 — 데미지 GE가 여기 값을 넣으면 PostGameplayEffectExecute가 받아서 실제
	// 체력·자세에 반영하고 0으로 리셋한다.
	UPROPERTY(BlueprintReadOnly, Category = "Damage")
	FGameplayAttributeData IncomingHealthDamage;
	ATTRIBUTE_ACCESSORS_BASIC(UBlademasterAttributeSet, IncomingHealthDamage)

	UPROPERTY(BlueprintReadOnly, Category = "Damage")
	FGameplayAttributeData IncomingPostureDamage;
	ATTRIBUTE_ACCESSORS_BASIC(UBlademasterAttributeSet, IncomingPostureDamage)
};
