#pragma once

#include "Abilities/GameplayAbility.h"
#include "CoreMinimal.h"
#include "BlademasterGameplayAbility_Guard.generated.h"

// 가드 입력(InputTag.Guard)을 누르고 있는 동안 방어 상태가 된다. 떼면 끝난다.
// 활성인 동안 State.Guard를 보유하고, 라우터(UBlademasterCombatComponent)가 이 태그로 가드 결과를 고른다.
// 이 어빌리티는 상태를 드러낼 뿐 막는 규칙(방향, 자세 소모)은 판단하지 않는다.
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

private:
	UFUNCTION()
	void OnGuardInputReleased(float TimeHeld);
};
