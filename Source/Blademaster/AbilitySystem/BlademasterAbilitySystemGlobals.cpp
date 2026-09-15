#include "AbilitySystem/BlademasterAbilitySystemGlobals.h"

#include "AbilitySystem/BlademasterGameplayEffectContext.h"

FGameplayEffectContext* UBlademasterAbilitySystemGlobals::AllocGameplayEffectContext() const
{
	return new FBlademasterGameplayEffectContext();
}
