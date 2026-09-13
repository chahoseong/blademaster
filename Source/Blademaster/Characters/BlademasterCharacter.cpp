#include "Characters/BlademasterCharacter.h"

#include "AbilitySystem/BlademasterAbilitySystemComponent.h"
#include "AbilitySystem/BlademasterAttributeSet.h"
#include "BlademasterGameplayTags.h"
#include "Combat/BlademasterCombatComponent.h"
#include "Combat/BlademasterWeaponTraceComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameplayEffect.h"
#include "GameplayTagContainer.h"
#include "MotionWarpingComponent.h"

#if !UE_BUILD_SHIPPING
#include "BlademasterDebug.h"
#endif

namespace
{
	const FName AttackDirectionWarpTargetName(TEXT("AttackDirection"));
}

ABlademasterCharacter::ABlademasterCharacter()
{
	SetNetUpdateFrequency(100.f);
	SetMinNetUpdateFrequency(2.f);

	AbilitySystemComponent = CreateDefaultSubobject<UBlademasterAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	AttributeSet = CreateDefaultSubobject<UBlademasterAttributeSet>(TEXT("AttributeSet"));

	// 판정은 서버에서만 도는데, 화면에 안 보이면 뼈 위치가 갱신되지 않아 칼 소켓·피직스 바디가
	// 제자리에 멈춘다. 그래서 서버·클라 구분 없이 항상 뼈를 갱신하도록 한다.
	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	GetMesh()->SetCollisionProfileName(FName(TEXT("BlademasterCharacterMesh")));

	WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(GetMesh(), TEXT("SOC_hand_r"));
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ShieldMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShieldMesh"));
	ShieldMesh->SetupAttachment(GetMesh(), TEXT("SOC_lowerarm_l"));
	ShieldMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	CombatComponent = CreateDefaultSubobject<UBlademasterCombatComponent>(TEXT("CombatComponent"));

	MotionWarpingComponent = CreateDefaultSubobject<UMotionWarpingComponent>(TEXT("MotionWarpingComponent"));

	WeaponTraceComponent = CreateDefaultSubobject<UBlademasterWeaponTraceComponent>(TEXT("WeaponTraceComponent"));
}

void ABlademasterCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 공격 어빌리티가 활성 상태인 동안, 방향 전환 구간에만 실제로 회전이 걸리도록
	// 워프 타깃을 계속 최신 방향으로 갱신해둔다(몽타주의 Motion Warping 노티파이가
	// 그 구간에서만 이 타깃을 실제로 사용한다). 위치는 항상 현재 위치라 이동 자체는 건드리지 않는다.
	if (MotionWarpingComponent && AbilitySystemComponent && AbilitySystemComponent->HasMatchingGameplayTag(BlademasterGameplayTags::State_Attacking))
	{
		MotionWarpingComponent->AddOrUpdateWarpTargetFromLocationAndRotation(AttackDirectionWarpTargetName, GetActorLocation(), GetAttackDirection());
	}

#if !UE_BUILD_SHIPPING
	if (BlademasterDebug::IsCombatDebugEnabled())
	{
		DrawOwnCombatDebugText();
		OnDrawDebug.Broadcast();
	}
#endif
}

UAbilitySystemComponent* ABlademasterCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

FRotator ABlademasterCharacter::GetAttackDirection() const
{
	return GetActorRotation();
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

	// 어빌리티 부여는 서버 권위 데이터라 서버에서만, 그리고 InitAbilityActorInfo 이후에 한다.
	// BeginPlay가 액터 생애주기당 한 번만 호출되므로(위 주석 참고) 여기서 주면 중복 부여가 없다.
	if (HasAuthority() && CombatComponent)
	{
		CombatComponent->GrantStartingAbilities();
	}

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

#if !UE_BUILD_SHIPPING
void ABlademasterCharacter::DrawOwnCombatDebugText() const
{
	if (AttributeSet)
	{
		BlademasterDebug::DrawDebugTextLine(this, 0, FString::Printf(TEXT("HP: %.0f / %.0f"), AttributeSet->GetHealth(), AttributeSet->GetMaxHealth()));
		BlademasterDebug::DrawDebugTextLine(this, 1, FString::Printf(TEXT("Posture: %.0f / %.0f"), AttributeSet->GetPosture(), AttributeSet->GetMaxPosture()));
	}

	if (AbilitySystemComponent)
	{
		// Attack.Window의 하위 태그를 전부 나열한다 — 새 구간 태그가 생겨도 이 코드는 그대로다.
		static const FGameplayTag WindowRootTag = FGameplayTag::RequestGameplayTag(FName("Attack.Window"));

		FGameplayTagContainer OwnedTags;
		AbilitySystemComponent->GetOwnedGameplayTags(OwnedTags);

		TArray<FString> ActiveWindowNames;
		for (const FGameplayTag& Tag : OwnedTags)
		{
			if (Tag != WindowRootTag && Tag.MatchesTag(WindowRootTag))
			{
				FString ParentPath, LeafName;
				Tag.ToString().Split(TEXT("."), &ParentPath, &LeafName, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
				ActiveWindowNames.Add(LeafName);
			}
		}

		const FString WindowText = ActiveWindowNames.Num() > 0 ? FString::Join(ActiveWindowNames, TEXT(", ")) : TEXT("-");
		BlademasterDebug::DrawDebugTextLine(this, 2, FString::Printf(TEXT("Window: %s"), *WindowText));
	}
}
#endif
