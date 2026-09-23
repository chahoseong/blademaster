#pragma once

#include "Abilities/GameplayAbility.h"
#include "CoreMinimal.h"
#include "BlademasterGameplayAbility_GuardReact.generated.h"

// GameplayEvent.Reaction.Guard를 받아 가드 반응 몽타주를 재생한다.
// 규칙 판단은 하지 않는다 — 가드 성립, 자세 소모, 공격자를 향한 회전은 라우터와 가드 어빌리티가 이미 끝냈고
// 이 어빌리티는 후보 중 하나를 골라 재생만 한다.
// 가드를 끊지 않아 가드를 유지한 채 계속 막을 수 있다. 재생 중에는 공격만 막는다 — 막은 뒤 반격하려면 반응이 끝나야 한다.
// 재생 중 다시 막으면 처음부터 다시 재생한다(bRetriggerInstancedAbility). 붕괴·사망이 이 어빌리티를 취소한다.
UCLASS()
class BLADEMASTER_API UBlademasterGameplayAbility_GuardReact : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UBlademasterGameplayAbility_GuardReact();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

private:
	UFUNCTION()
	void OnMontageEnded();
};
