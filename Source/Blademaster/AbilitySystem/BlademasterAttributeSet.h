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
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;

	UPROPERTY(BlueprintReadOnly, Category = "Health", ReplicatedUsing = OnRep_Health)
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS_BASIC(UBlademasterAttributeSet, Health)

	UPROPERTY(BlueprintReadOnly, Category = "Health", ReplicatedUsing = OnRep_MaxHealth)
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS_BASIC(UBlademasterAttributeSet, MaxHealth)

	UPROPERTY(BlueprintReadOnly, Category = "Posture", ReplicatedUsing = OnRep_Posture)
	FGameplayAttributeData Posture;
	ATTRIBUTE_ACCESSORS_BASIC(UBlademasterAttributeSet, Posture)

	UPROPERTY(BlueprintReadOnly, Category = "Posture", ReplicatedUsing = OnRep_MaxPosture)
	FGameplayAttributeData MaxPosture;
	ATTRIBUTE_ACCESSORS_BASIC(UBlademasterAttributeSet, MaxPosture)

	UPROPERTY(BlueprintReadOnly, Category = "Posture", ReplicatedUsing = OnRep_BasePostureRegenRate)
	FGameplayAttributeData BasePostureRegenRate;
	ATTRIBUTE_ACCESSORS_BASIC(UBlademasterAttributeSet, BasePostureRegenRate)

protected:
	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldHealth);

	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth);

	UFUNCTION()
	void OnRep_Posture(const FGameplayAttributeData& OldPosture);

	UFUNCTION()
	void OnRep_MaxPosture(const FGameplayAttributeData& OldMaxPosture);

	UFUNCTION()
	void OnRep_BasePostureRegenRate(const FGameplayAttributeData& OldBasePostureRegenRate);
};
