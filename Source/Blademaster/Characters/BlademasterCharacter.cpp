#include "Characters/BlademasterCharacter.h"

#include "AbilitySystem/BlademasterAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameplayEffect.h"

ABlademasterCharacter::ABlademasterCharacter()
{
	SetNetUpdateFrequency(100.f);
	SetMinNetUpdateFrequency(2.f);

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	AttributeSet = CreateDefaultSubobject<UBlademasterAttributeSet>(TEXT("AttributeSet"));
}

UAbilitySystemComponent* ABlademasterCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ABlademasterCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
		AbilitySystemComponent->SetReplicationMode(Cast<APlayerController>(NewController)
			? EGameplayEffectReplicationMode::Mixed
			: EGameplayEffectReplicationMode::Minimal);

		if (HasAuthority() && InitializeAttributesEffect)
		{
			FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
			EffectContext.AddSourceObject(this);

			const FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(InitializeAttributesEffect, 1.f, EffectContext);
			if (SpecHandle.IsValid())
			{
				AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
			}
		}
	}
}
