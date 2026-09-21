#include "Development/BlademasterAttackDummy.h"

#include "AbilitySystem/BlademasterAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "BlademasterLogChannels.h"
#include "Components/SceneComponent.h"
#include "GameplayEffect.h"

#if !UE_BUILD_SHIPPING
#include "BlademasterDebug.h"
#endif

ABlademasterAttackDummy::ABlademasterAttackDummy()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));

	AttributeSet = CreateDefaultSubobject<UBlademasterAttributeSet>(TEXT("AttributeSet"));
}

void ABlademasterAttackDummy::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

#if !UE_BUILD_SHIPPING
	DrawAttributeText();
#endif
}

UAbilitySystemComponent* ABlademasterAttackDummy::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ABlademasterAttackDummy::BeginPlay()
{
	Super::BeginPlay();

	if (!InitializeAttributesEffect)
	{
		UE_LOG(LogBlademasterCombat, Warning, TEXT("%s: InitializeAttributesEffect가 지정되지 않아 체력·자세가 0으로 남는다"), *GetName());
		return;
	}

	FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
	EffectContext.AddSourceObject(this);

	const FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(InitializeAttributesEffect, 1.f, EffectContext);
	if (SpecHandle.IsValid())
	{
		AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
}

#if !UE_BUILD_SHIPPING
void ABlademasterAttackDummy::DrawAttributeText() const
{
	const UBlademasterAttributeSet* RegisteredSet = AbilitySystemComponent ? AbilitySystemComponent->GetSet<UBlademasterAttributeSet>() : nullptr;
	if (!RegisteredSet)
	{
		BlademasterDebug::DrawDebugTextLine(this, 0, TEXT("AttributeSet 미등록"), FColor::Red);
		return;
	}

	BlademasterDebug::DrawDebugTextLine(this, 0, FString::Printf(TEXT("HP: %.0f / %.0f"), RegisteredSet->GetHealth(), RegisteredSet->GetMaxHealth()));
	BlademasterDebug::DrawDebugTextLine(this, 1, FString::Printf(TEXT("Posture: %.0f / %.0f"), RegisteredSet->GetPosture(), RegisteredSet->GetMaxPosture()));
}
#endif
