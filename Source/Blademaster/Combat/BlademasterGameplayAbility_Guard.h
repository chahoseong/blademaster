#pragma once

#include "Abilities/GameplayAbility.h"
#include "CoreMinimal.h"
#include "BlademasterGameplayAbility_Guard.generated.h"

// 가드 입력(InputTag.Guard)을 누르고 있는 동안 방어 상태가 된다. 떼면 끝난다.
// 활성인 동안 State.Guard를 보유하고, 라우터(UBlademasterCombatComponent)에 자신을 활성 가드로 알린다.
// 라우터는 타격을 해석할 때 이 가드에 그 공격을 막을 수 있는지 묻고, 결과(가드/피격 등)는 라우터가 정한다.
// 이 어빌리티가 답하는 것은 가드의 유효 범위 — 공격자가 정면 ±FrontHalfAngle 안에 있는지 — 뿐이다.
//
// Ability.Action.Guard라서 피격 반응·붕괴·사망이 끊고 막는다. 끊긴 뒤에는 누르고 있어도 다시 눌러야 가드한다 —
// 붕괴 중에 들어온 가드 입력이 붕괴가 끝난 뒤 자동으로 성립하지 않는 것(Specs/002-stagger.md R-3)도 이 때문이다.
// 가드 중에는 공격이 막히고, 공격 중에는 가드가 시작되지 않는다.
UCLASS()
class BLADEMASTER_API UBlademasterGameplayAbility_Guard : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UBlademasterGameplayAbility_Guard();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	// AttackerLocation에서 온 공격이 이 가드의 유효 범위 안인지. 가드하는 캐릭터의 현재 위치·방향으로 판단한다.
	bool CanBlockAttackFrom(const FVector& AttackerLocation) const;

	// 수평면에서 공격자가 피격자 정면 ±HalfAngleDegrees 안에 있으면 true. 경계는 포함한다.
	// 높이 차이는 무시하고, 두 위치가 겹쳐 방향이 없으면 정면으로 본다.
	static bool IsWithinFrontArc(const FVector& VictimLocation, const FVector& VictimForward, const FVector& AttackerLocation, float HalfAngleDegrees);

protected:
	// 가드가 막을 수 있는 범위의 반각. 60이면 정면 좌우 60°(총 120°)다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Guard", meta = (ClampMin = "0", ClampMax = "180", Units = "Degrees"))
	float FrontHalfAngle = 60.f;

private:
	UFUNCTION()
	void OnGuardInputReleased(float TimeHeld);
};
