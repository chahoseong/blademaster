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

void ABlademasterCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (!AbilitySystemComponent)
	{
		return;
	}

	// OwnerActor/AvatarActor가 항상 this라 컨트롤러 빙의와 무관하게 안전하다.
	// 액터 생애주기당 한 번만 호출되므로 어트리뷰트 초기화 GE도 여기서 한 번만 적용한다.
	AbilitySystemComponent->InitAbilityActorInfo(this, this);

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

void ABlademasterCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->SetReplicationMode(Cast<APlayerController>(NewController)
			? EGameplayEffectReplicationMode::Mixed
			: EGameplayEffectReplicationMode::Minimal);
	}
}
