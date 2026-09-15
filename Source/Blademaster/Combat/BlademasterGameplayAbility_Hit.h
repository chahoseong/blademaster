#pragma once

#include "Abilities/GameplayAbility.h"
#include "CoreMinimal.h"
#include "BlademasterGameplayAbility_Hit.generated.h"

class UGameplayEffect;

// 판정(#12)이 GameplayEvent.WeaponHit으로 보낸 결과를 받아 발동한다. 공격자의 ASC로 데미지 GE
// 스펙을 만든다 — 공격 데이터(체력·자세 데미지)는 이벤트가 실어온 컨텍스트에 이미 담겨 있다.
// 막기·회피가 끼어들 자리(①)는 지금 비워둔다 — 데미지 계산이 아니라 이 어빌리티 자체가 M2에서 맡는다.
UCLASS()
class BLADEMASTER_API UBlademasterGameplayAbility_Hit : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UBlademasterGameplayAbility_Hit();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Hit")
	TSubclassOf<UGameplayEffect> DamageEffectClass;
};
