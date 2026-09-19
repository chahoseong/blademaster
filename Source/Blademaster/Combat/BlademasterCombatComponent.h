#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "BlademasterCombatComponent.generated.h"

class UGameplayAbility;

// 시작 시 부여할 어빌리티 하나와, 그 어빌리티를 입력으로 트리거할 때 쓰는 태그.
// 태그는 부여 시점에 어빌리티 스펙의 DynamicSpecSourceTags에 붙는다(입력 라우팅 전용,
// 어빌리티 자체의 정체성 태그인 AssetTags/AbilityTags와는 별개).
USTRUCT(BlueprintType)
struct FBlademasterCombatAbilityToGrant
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	TSubclassOf<UGameplayAbility> AbilityClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	FGameplayTag InputTag;
};

// 전투 관련 어빌리티 부여와, 입력 태그를 ASC 입력 경로로 라우팅하는 책임을 맡는다.
// 플레이어 전용이 아니다 — AI도 같은 경로로 어빌리티를 트리거할 수 있다.
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class BLADEMASTER_API UBlademasterCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBlademasterCombatComponent();

	// 액터 생애주기당 한 번만 호출한다. ASC의 InitAbilityActorInfo가 끝난 뒤 호출해야 한다.
	void GrantStartingAbilities();

	void AbilityInputTagPressed(const FGameplayTag& InputTag);
	void AbilityInputTagReleased(const FGameplayTag& InputTag);

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	TArray<FBlademasterCombatAbilityToGrant> StartingAbilities;
};
