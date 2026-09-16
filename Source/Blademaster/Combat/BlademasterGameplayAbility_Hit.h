#pragma once

#include "Abilities/GameplayAbility.h"
#include "CoreMinimal.h"
#include "BlademasterGameplayAbility_Hit.generated.h"

class UGameplayEffect;
enum class EBlademasterHitDirection : uint8;
struct FHitResult;

// 판정(#12)이 GameplayEvent.WeaponHit으로 보낸 결과를 받아 발동한다. 공격자의 ASC로 데미지 GE
// 스펙을 만든다 — 공격 데이터(체력·자세 데미지)는 이벤트가 실어온 컨텍스트에 이미 담겨 있다.
// 체력이 0이 되면 State.Dead를 붙이고 방향별 사망 몽타주를, 아니면 방향별 피격 몽타주를 재생한다.
// 재생 중 다시 맞으면 처음부터 다시 재생한다(bRetriggerInstancedAbility). 하던 공격은 취소되고,
// 재생이 끝날 때까지 새 공격은 막힌다(Ability.Action 태그 기반, 콤보 외의 미래 공격류도 자동 적용).
// 막기·회피가 끼어들 자리(①)는 지금 비워둔다 — M2에서 이 어빌리티 자체가 맡는다.
UCLASS()
class BLADEMASTER_API UBlademasterGameplayAbility_Hit : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UBlademasterGameplayAbility_Hit();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

private:
	// 판정이 넘겨준 칼의 이동 방향(스윕 시작->끝)을 피격자 기준 좌/우/정면으로 바꾼다.
	static EBlademasterHitDirection ComputeHitDirection(const FHitResult& Hit, const AActor* Victim);

	UFUNCTION()
	void OnHitReactMontageEnded();

	UFUNCTION()
	void OnDeathMontageEnded();

	// BlockAbilitiesWithTags와 UnBlockAbilitiesWithTags를 항상 짝 맞춰 호출하기 위한 상태.
	// InstancedPerActor + 재트리거라 같은 인스턴스가 재사용되므로 ActivateAbility 시작 시 초기화한다.
	bool bBlockedActionAbilities = false;
};
