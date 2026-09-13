#pragma once

#include "Abilities/GameplayAbility.h"
#include "CoreMinimal.h"
#include "BlademasterGameplayAbility_Combo.generated.h"

class UBlademasterComboDefinition;

// 기본 콤보의 첫 타 몽타주를 재생하고 끝나면 종료한다. 이어가기는 M1-4에서 추가한다.
UCLASS()
class BLADEMASTER_API UBlademasterGameplayAbility_Combo : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UBlademasterGameplayAbility_Combo();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combo")
	TObjectPtr<UBlademasterComboDefinition> ComboDefinition;

private:
	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageInterrupted();
};
