#pragma once

#include "Abilities/GameplayAbility.h"
#include "CoreMinimal.h"
#include "BlademasterGameplayAbility_HitReact.generated.h"

// GameplayEvent.Reaction.Hit을 받아 피격 반응 몽타주를 재생한다.
// 규칙 판단은 하지 않는다 — 데미지 적용, 사망 여부, 피격 방향은 UBlademasterCombatComponent가 이미 정했고
// 이 어빌리티는 이벤트 컨텍스트의 방향으로 몽타주만 고른다.
// 하던 공격은 취소되고, 재생이 끝날 때까지 새 공격은 막힌다(Ability.Action을 취소·차단 선언으로 지정).
// 재생 중 다시 맞으면 처음부터 다시 재생한다(bRetriggerInstancedAbility).
UCLASS()
class BLADEMASTER_API UBlademasterGameplayAbility_HitReact : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UBlademasterGameplayAbility_HitReact();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

private:
	UFUNCTION()
	void OnMontageEnded();
};
