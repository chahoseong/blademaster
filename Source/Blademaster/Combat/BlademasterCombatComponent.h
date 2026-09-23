#pragma once

#include "Combat/BlademasterHitDirection.h"
#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "BlademasterCombatComponent.generated.h"

class UAbilitySystemComponent;
class UBlademasterGameplayAbility_Guard;
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
// 이미 죽었으면 무시하고, 활성 가드가 그 공격을 막는지 물어 적용할 데미지 GE(가드/일반)를 고르고, 적용한 뒤
// 확정된 체력·자세로 결과(사망/붕괴 진입/가드/피격)를 정하고, 피격 방향을 구한 뒤 결과를
// GameplayEvent.Reaction.Death/Stagger/Guard/Hit으로 자신에게 보낸다. 붕괴 중에 맞으면 체력만 깎이고 반응 이벤트는 보내지 않는다.
//
// 가드: 가드 어빌리티가 활성인 동안 SetActiveGuard로 자신을 알린다. 막을 수 있는 범위는 가드가 답하고, 결과는 여기서 정한다.
// 결과가 가드면 OnAttackBlocked로 가드에 알린다.
// 반응 어빌리티(GA_HitReact, GA_Death, GA_Stagger, GA_GuardReact)는 그 결과를 재생만 하고 규칙 판단은 하지 않는다.
//
// 자세 회복: 시작할 때 무한 주기 회복 GE를 적용하고, 타격으로 자세가 줄면 회복 대기 GE를 적용한다.
// 대기 GE가 붙이는 State.Posture.RegenDelay가 있는 동안 회복 GE가 멈추고, 다시 적용하면 대기가 처음부터 다시 센다.
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

	// 자세 회복 GE를 적용한다. 초기화 GE를 적용한 뒤 한 번만 호출한다. 되살아날 때는 다시 부르지 않는다 — 회복 GE는 무한 GE라 남아 있다.
	void ApplyPostureRegen();

	// 타격을 해석할 때 물어볼 활성 가드. 가드 어빌리티가 활성화될 때 설정한다.
	void SetActiveGuard(UBlademasterGameplayAbility_Guard* Guard);

	// Guard가 지금 활성 가드일 때만 비운다. 가드가 끝나자마자 다시 켜져도 새 가드를 지우지 않는다.
	void ClearActiveGuard(const UBlademasterGameplayAbility_Guard* Guard);

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

	// 가드로 막았을 때 적용하는 데미지 GE. 자세만 깎는다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Hit", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UGameplayEffect> GuardDamageEffectClass;

	// 자세를 주기적으로 회복하는 무한 GE. State.Posture.RegenDelay가 있는 동안 멈춘다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Posture", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UGameplayEffect> PostureRegenEffectClass;

	// 자세가 깎였을 때 적용하는 회복 대기 GE. 지속 시간 동안 State.Posture.RegenDelay를 붙인다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Posture", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UGameplayEffect> PostureRegenDelayEffectClass;

private:
	void OnWeaponHitEvent(FGameplayTag EventTag, const FGameplayEventData* Payload);

	// 자세가 줄었을 때 회복 대기를 (다시) 시작한다.
	void StartPostureRegenDelay(UAbilitySystemComponent* AbilitySystemComponent);

	void OnPostureRegenDelayTagChanged(const FGameplayTag Tag, int32 NewCount);

	TWeakObjectPtr<UBlademasterGameplayAbility_Guard> ActiveGuard;

	FDelegateHandle WeaponHitEventHandle;
	FDelegateHandle PostureRegenDelayTagHandle;
};
