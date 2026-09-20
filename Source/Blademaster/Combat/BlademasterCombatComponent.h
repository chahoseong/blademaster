#pragma once

#include "Combat/BlademasterHitDirection.h"
#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "BlademasterCombatComponent.generated.h"

class UGameplayAbility;
class UGameplayEffect;
struct FGameplayEventData;
struct FHitResult;

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

// 전투 관련 어빌리티 부여, 입력 태그를 ASC 입력 경로로 라우팅, 타격 이벤트 라우팅을 맡는다.
// 플레이어 전용이 아니다 — AI도 같은 경로로 어빌리티를 트리거할 수 있다.
//
// 타격 라우팅: 공격자가 보낸 GameplayEvent.Weapon.Hit을 받아 피격자 쪽의 규칙을 순서대로 처리한다.
// 이미 죽었으면 무시하고, 데미지 GE를 적용하고, 확정된 체력으로 결과(피격/사망)를 정하고,
// 피격 방향을 구한 뒤 결과를 GameplayEvent.Reaction.Hit/Death로 자신에게 보낸다.
// 반응 어빌리티(GA_HitReact, GA_Death)는 그 결과를 재생만 하고 규칙 판단은 하지 않는다.
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class BLADEMASTER_API UBlademasterCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBlademasterCombatComponent();

	// 액터 생애주기당 한 번만 호출한다. ASC의 InitAbilityActorInfo가 끝난 뒤 호출해야 한다.
	void GrantStartingAbilities();

	// 타격 이벤트 구독을 시작한다. GrantStartingAbilities와 마찬가지로 ASC 초기화가 끝난 뒤 한 번만 호출한다.
	void StartListeningForHits();

	void AbilityInputTagPressed(const FGameplayTag& InputTag);
	void AbilityInputTagReleased(const FGameplayTag& InputTag);

	// 칼의 이동 방향(스윕 시작->끝)을 피격자 기준 좌/우/정면으로 바꾼다. VictimRight는 피격자의 오른쪽 벡터.
	// 칼이 피격자의 오른쪽으로 움직이며 지나갔다는 건 반대편(왼쪽)에서 걸어들어왔다는 뜻이다.
	static EBlademasterHitDirection ComputeHitDirection(const FHitResult& Hit, const FVector& VictimRight);

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	TArray<FBlademasterCombatAbilityToGrant> StartingAbilities;

	// 타격 한 번에 체력·자세를 깎는 데미지 GE. 스펙은 공격자의 ASC로 만든다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Hit", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UGameplayEffect> DamageEffectClass;

private:
	void OnWeaponHitEvent(FGameplayTag EventTag, const FGameplayEventData* Payload);

	FDelegateHandle WeaponHitEventHandle;
};
