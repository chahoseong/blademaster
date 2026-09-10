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
	//
	// 주의: InitAbilityActorInfo는 여기서만 호출한다(PossessedBy에는 없음). 그래서
	// AbilityActorInfo->PlayerController는 이 최초 호출 시점의 컨트롤러로 고정되고,
	// 이후 같은 액터가 다른 컨트롤러에 "재빙의"돼도 갱신되지 않는다(UAbilitySystemComponent::
	// RefreshAbilityActorInfo를 PossessedBy에서 불러야 갱신됨). 지금(M0~M2) 로드맵 범위에서는
	// 재빙의 시나리오(같은 Pawn을 죽이지 않고 다른 컨트롤러가 다시 빙의하는 경우 — 컷신 카메라
	// 전환 후 복귀, 관전 모드 등) 자체가 없어서 문제되지 않는다. 리스폰은 보통 죽은 Pawn을
	// Destroy하고 새 Pawn을 스폰하는 방식이라 "재빙의"가 아니라 매번 새 액터의 최초 빙의다.
	// 나중에 재빙의가 생기는 기능(컷신, 관전 등)을 추가하게 되면 PossessedBy에도
	// InitAbilityActorInfo(또는 RefreshAbilityActorInfo)를 추가해야 한다.
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
