#pragma once

#include "Abilities/GameplayAbility.h"
#include "CoreMinimal.h"
#include "BlademasterGameplayAbility_Death.generated.h"

// GameplayEvent.Reaction.Death를 받아 사망 시퀀스를 처음부터 끝까지 소유한다.
// State.Death.Dying 부여 -> 무기 채널 충돌 끄기 -> 사망 몽타주 재생 -> State.Death.Dead 전환.
// 규칙 판단은 하지 않는다 — 사망이라는 결과는 UBlademasterCombatComponent가 이미 정했다.
// GA_Respawn이 State.Death.Dead를 트리거로 받으므로, 이 전환 시점이 부활 대기시간의 시작점이다.
// 하던 공격은 취소되고 새 공격은 막힌다(Ability.Action을 취소·차단 선언으로 지정).
// 취소되면 State.Death.Dying이 남을 수 있다 — 문제가 되면 상태 전환을 CombatComponent로 옮긴다.
UCLASS()
class BLADEMASTER_API UBlademasterGameplayAbility_Death : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UBlademasterGameplayAbility_Death();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

private:
	UFUNCTION()
	void OnDeathMontageEnded();
};
