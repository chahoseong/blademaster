#pragma once

#include "Abilities/GameplayAbility.h"
#include "CoreMinimal.h"
#include "BlademasterGameplayAbility_Death.generated.h"

// GameplayEvent.Reaction.Death를 받아 사망 상태 전체를 소유한다.
// State.Death.Dying 부여 -> 무기 채널 충돌 끄기 -> 사망 몽타주 재생 -> State.Death.Dead 전환.
// 규칙 판단은 하지 않는다 — 사망이라는 결과는 UBlademasterCombatComponent가 이미 정했다.
//
// 어빌리티의 수명이 곧 사망 상태의 수명이다. Dead로 넘어간 뒤에도 스스로 끝나지 않고, 밖에서
// 취소할 때까지(되살리기 등) 활성 상태로 남는다. 그래서 Ability.Action 취소·차단 선언 하나가 사망
// 전체 구간을 덮고, 사망 몽타주도 어빌리티가 끝날 때 함께 내려간다. 끝날 때(EndAbility) 두 단계 태그를
// 조건 없이 걷어내므로 어느 단계에서 취소되든 태그가 남지 않는다.
// GA_Respawn이 State.Death.Dead를 트리거로 받으므로, 이 전환 시점이 부활 대기시간의 시작점이다.
// 무기 채널 충돌은 여기서 끄고 GA_Respawn이 다시 켠다 — GA_Respawn 외의 경로로 끝나면 꺼진 채 남는다.
UCLASS()
class BLADEMASTER_API UBlademasterGameplayAbility_Death : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UBlademasterGameplayAbility_Death();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	UFUNCTION()
	void OnDeathMontageEnded();
};
