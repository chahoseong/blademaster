#pragma once

#include "AbilitySystemGlobals.h"
#include "CoreMinimal.h"
#include "BlademasterAbilitySystemGlobals.generated.h"

// DefaultGame.ini의 AbilitySystemGlobalsClassName으로 등록해서, 프로젝트 전역에서
// FGameplayEffectContext 대신 FBlademasterGameplayEffectContext가 쓰이게 한다.
UCLASS()
class BLADEMASTER_API UBlademasterAbilitySystemGlobals : public UAbilitySystemGlobals
{
	GENERATED_BODY()

public:
	virtual FGameplayEffectContext* AllocGameplayEffectContext() const override;
};
