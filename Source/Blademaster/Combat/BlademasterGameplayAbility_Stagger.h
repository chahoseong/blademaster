#pragma once

#include "Abilities/GameplayAbility.h"
#include "CoreMinimal.h"
#include "BlademasterGameplayAbility_Stagger.generated.h"

class UGameplayEffect;

// GameplayEvent.Reaction.Stagger를 받아 붕괴 상태 전체를 소유한다(Specs/002-stagger.md).
// 규칙 판단은 하지 않는다 — 붕괴 진입은 UBlademasterCombatComponent가 이미 정했다.
//
// 활성인 동안 State.Stagger를 보유하고, Ability.Action(공격, 이후의 가드)과 피격 반응을 취소·차단하며,
// 캐릭터의 이동과 몸 회전을 끈다(R-3, R-4). 지속 시간은 진입할 때의 StaggerDuration으로 정해지고
// 이후의 타격이 바꾸지 않는다(R-5). 시간이 다 되면 자세를 최대치로 되돌리는 GE를 적용하고 끝난다(R-6).
// 사망(GA_Death)이 취소하면 복원 GE 없이 끝난다 — 태그와 이동 금지는 어느 쪽으로 끝나든 걷힌다.
UCLASS()
class BLADEMASTER_API UBlademasterGameplayAbility_Stagger : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UBlademasterGameplayAbility_Stagger();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	// 붕괴가 끝나는 순간 자세를 최대치로 되돌리는 GE.
	UPROPERTY(EditDefaultsOnly, Category = "Stagger")
	TSubclassOf<UGameplayEffect> PostureRestoreEffectClass;

private:
	UFUNCTION()
	void OnStaggerDurationElapsed();
};
