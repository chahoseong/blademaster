#include "Combat/TargetableComponent.h"

#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "BlademasterGameplayTags.h"

bool UTargetableComponent::IsTargetable() const
{
	const IAbilitySystemInterface* AbilitySystemInterface = Cast<IAbilitySystemInterface>(GetOwner());
	const UAbilitySystemComponent* AbilitySystemComponent = AbilitySystemInterface ? AbilitySystemInterface->GetAbilitySystemComponent() : nullptr;
	if (!AbilitySystemComponent)
	{
		return false;
	}

	return !AbilitySystemComponent->HasMatchingGameplayTag(BlademasterGameplayTags::State_Dying)
		&& !AbilitySystemComponent->HasMatchingGameplayTag(BlademasterGameplayTags::State_Dead);
}
