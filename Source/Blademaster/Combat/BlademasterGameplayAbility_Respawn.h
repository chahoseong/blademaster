#pragma once

#include "Abilities/GameplayAbility.h"
#include "CoreMinimal.h"
#include "BlademasterGameplayAbility_Respawn.generated.h"

// 쓰러짐(State.Death.Dead)이 붙는 순간 발동한다(OwnedTagAdded — 태그가 떨어져도 취소되지 않는다).
// 설정 시간만큼 기다렸다가 초기화 GE를 다시 적용하고 State.Death.Dead를 없앤다. GA_Death는 사망 시퀀스를 끝내고
// State.Death.Dead를 붙이는 것까지만 하고 부활은 완전히 모른다 — 태그로만 연결되는 구조다.
// 더미에게만 부여한다(플레이어의 패배·재시작은 M5). 부활 시간은 이 어빌리티 자체의 설정값이다.
UCLASS()
class BLADEMASTER_API UBlademasterGameplayAbility_Respawn : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UBlademasterGameplayAbility_Respawn();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Respawn")
	float RespawnDelay = 3.f;

private:
	UFUNCTION()
	void OnRespawnDelayFinished();
};
