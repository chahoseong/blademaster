#pragma once

#include "Abilities/GameplayAbility.h"
#include "CoreMinimal.h"
#include "BlademasterGameplayAbility_Respawn.generated.h"

// 검증용 더미 되살리기다. 더미를 반복해서 때리며 판정·피해·사망을 확인하는 도구이고, 전투 판의 재시도
// (시작·승리·패배·재시도)는 M5에서 별도로 만든다. 그때 이 어빌리티의 제거를 검토한다.
//
// 쓰러짐(State.Death.Dead)이 붙는 순간 발동한다(OwnedTagAdded — 태그가 떨어져도 취소되지 않는다). 그 시점이
// 사망 연출이 끝난 시점이므로 RespawnDelay는 연출이 끝난 뒤 기다리는 시간이다.
// 대기가 끝나면 되돌릴 것을 자기 안에서 되돌리고(무기 채널 충돌 켜기, 초기화 GE 재적용), 마지막에 GA_Death를
// 취소한다. 사망 태그와 사망 몽타주의 정리는 GA_Death가 끝날 때 하므로 여기서 하지 않는다.
// 앞으로 되돌릴 목록에 지속 GE 제거가 추가된다 — 자세 회복 지연처럼 지속 GE가 붙으면 죽은 채로 남는다.
// 캐릭터 BeginPlay의 초기화와 로직을 공유하지 않는다. 위치는 복구하지 않는다(죽은 자리에서 일어난다).
// 더미에게만 부여한다. 부활 시간은 이 어빌리티 자체의 설정값이다.
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
