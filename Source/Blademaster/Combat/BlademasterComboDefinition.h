#pragma once

#include "Combat/BlademasterAttackDefinition.h"
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "BlademasterComboDefinition.generated.h"

UCLASS()
class BLADEMASTER_API UBlademasterComboDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combo")
	TArray<FBlademasterAttackDefinition> Attacks;
};
