#include "Combat/BlademasterCombatComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Abilities/GameplayAbility.h"

UBlademasterCombatComponent::UBlademasterCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UBlademasterCombatComponent::GrantStartingAbilities()
{
	AActor* Owner = GetOwner();
	const IAbilitySystemInterface* AbilitySystemInterface = Cast<IAbilitySystemInterface>(Owner);
	UAbilitySystemComponent* AbilitySystemComponent = AbilitySystemInterface ? AbilitySystemInterface->GetAbilitySystemComponent() : nullptr;
	if (!AbilitySystemComponent)
	{
		return;
	}

	for (const FBlademasterCombatAbilityToGrant& Grant : StartingAbilities)
	{
		if (!Grant.AbilityClass)
		{
			continue;
		}

		FGameplayAbilitySpec Spec(Grant.AbilityClass, 1, INDEX_NONE, Owner);
		Spec.GetDynamicSpecSourceTags().AddTag(Grant.InputTag);
		AbilitySystemComponent->GiveAbility(Spec);
	}
}

void UBlademasterCombatComponent::AbilityInputTagPressed(const FGameplayTag& InputTag)
{
	const IAbilitySystemInterface* AbilitySystemInterface = Cast<IAbilitySystemInterface>(GetOwner());
	UAbilitySystemComponent* AbilitySystemComponent = AbilitySystemInterface ? AbilitySystemInterface->GetAbilitySystemComponent() : nullptr;
	if (!AbilitySystemComponent || !InputTag.IsValid())
	{
		return;
	}

	for (FGameplayAbilitySpec& Spec : AbilitySystemComponent->GetActivatableAbilities())
	{
		if (!Spec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
		{
			continue;
		}

		AbilitySystemComponent->AbilitySpecInputPressed(Spec);
		if (!Spec.IsActive())
		{
			AbilitySystemComponent->TryActivateAbility(Spec.Handle);
		}
	}
}

void UBlademasterCombatComponent::AbilityInputTagReleased(const FGameplayTag& InputTag)
{
	const IAbilitySystemInterface* AbilitySystemInterface = Cast<IAbilitySystemInterface>(GetOwner());
	UAbilitySystemComponent* AbilitySystemComponent = AbilitySystemInterface ? AbilitySystemInterface->GetAbilitySystemComponent() : nullptr;
	if (!AbilitySystemComponent || !InputTag.IsValid())
	{
		return;
	}

	for (FGameplayAbilitySpec& Spec : AbilitySystemComponent->GetActivatableAbilities())
	{
		if (Spec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
		{
			AbilitySystemComponent->AbilitySpecInputReleased(Spec);
		}
	}
}
