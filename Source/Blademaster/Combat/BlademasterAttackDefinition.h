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
};
