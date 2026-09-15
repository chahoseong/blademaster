#include "AbilitySystem/BlademasterGameplayEffectContext.h"

bool FBlademasterGameplayEffectContext::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	Super::NetSerialize(Ar, Map, bOutSuccess);

	Ar << HealthDamage;
	Ar << PostureDamage;

	bOutSuccess = true;
	return true;
}
