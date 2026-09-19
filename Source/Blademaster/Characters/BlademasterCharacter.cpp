#include "Characters/BlademasterCharacter.h"

#include "AbilitySystem/BlademasterAbilitySystemComponent.h"
#include "AbilitySystem/BlademasterAttributeSet.h"
#include "BlademasterCollisionChannels.h"
#include "BlademasterGameplayTags.h"
#include "Combat/BlademasterCombatComponent.h"
#include "Combat/BlademasterWeaponTraceComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
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
	AbilitySystemComponent = CreateDefaultSubobject<UBlademasterAbilitySystemComponent>(TEXT("AbilitySystemComponent"));

	AttributeSet = CreateDefaultSubobject<UBlademasterAttributeSet>(TEXT("AttributeSet"));

	// 화면 밖에서도 판정이 돌아야 하는데, 화면에 안 보이면 뼈 위치가 갱신되지 않아 칼 소켓·피직스
	// 바디가 제자리에 멈춘다. 그래서 항상 뼈를 갱신하도록 한다.
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

const TArray<TObjectPtr<UAnimMontage>>& ABlademasterCharacter::GetHitReactMontages(EBlademasterHitDirection Direction) const
{
	switch (Direction)
	{
	case EBlademasterHitDirection::Left:
		return HitReactMontages_Left;
	case EBlademasterHitDirection::Right:
		return HitReactMontages_Right;
	default:
		return HitReactMontages_Front;
	}
}

UAnimMontage* ABlademasterCharacter::GetDeathMontage(EBlademasterHitDirection Direction) const
{
	return Direction == EBlademasterHitDirection::Right ? DeathMontage_Right : DeathMontage_Left;
}

void ABlademasterCharacter::SetCombatCollisionEnabled(bool bEnabled)
{
	GetMesh()->SetCollisionResponseToChannel(BlademasterCollisionChannels::Weapon, bEnabled ? ECR_Overlap : ECR_Ignore);
}

void ABlademasterCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (!AbilitySystemComponent)
	{
		return;
	}

	if (CombatComponent)
	{
		CombatComponent->GrantStartingAbilities();
	}

	if (InitializeAttributesEffect)
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

void ABlademasterCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	// InitAbilityActorInfo가 캐싱한 PlayerController는 캐릭터에 새로운 컨트롤러가
	// 연결되더라도, 갱신이 되지 않습니다. 따라서 RefreshAbilityActorInfo를 호출하여 캐시된 
	// 컨트롤러를 갱신합니다.InitAbilityActorInfo를 다시 호출할 경우, 몽타주 복제 상태가 리셋이 되어
	// 컷신 이후 복귀 또는 관전 모드처럼 게임플레이 도중에 컨트롤러가 변경될 경우, 문제가 될 수 있습니다.
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->RefreshAbilityActorInfo();
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
		const FGameplayTag WindowRootTag = BlademasterGameplayTags::Attack_Window;

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
