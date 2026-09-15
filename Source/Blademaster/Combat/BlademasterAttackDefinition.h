#pragma once

#include "CoreMinimal.h"
#include "BlademasterAttackDefinition.generated.h"

class UAnimMontage;

USTRUCT(BlueprintType)
struct FBlademasterAttackDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
	TObjectPtr<UAnimMontage> Montage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
	float HealthDamage = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
	float PostureDamage = 0.f;
};
