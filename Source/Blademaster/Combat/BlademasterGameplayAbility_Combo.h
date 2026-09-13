#pragma once

#include "Abilities/GameplayAbility.h"
#include "CoreMinimal.h"
#include "BlademasterGameplayAbility_Combo.generated.h"

class UAbilityTask_PlayMontageAndWait;
class UAbilityTask_WaitInputPress;
class UBlademasterComboDefinition;

// 콤보 데이터를 순서대로 재생한다. 각 타마다 선입력·이어가기 구간 태그를 지켜보다가
// 조건이 맞으면 다음 타로 넘어가고, 마지막 타까지 가거나 이어가기를 놓치면 끝난다.
// 방향 전환은 이 어빌리티가 아니라 Motion Warping(캐릭터 Tick)이 맡는다.
// 캔슬은 이 어빌리티가 하지 않는다 — 캔슬 구간 태그를 보고 다른 어빌리티가 끊는다.
UCLASS()
class BLADEMASTER_API UBlademasterGameplayAbility_Combo : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UBlademasterGameplayAbility_Combo();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combo")
	TObjectPtr<UBlademasterComboDefinition> ComboDefinition;

private:
	// Index번째 공격의 몽타주를 재생하고, 마지막 타가 아니면 이어가기 감지를 건다.
	void PlayAttack(int32 Index);

	// WaitInputPress는 한 번 발동하면 끝나는 태스크라, 계속 입력을 받으려면 매번 다시 걸어야 한다.
	void ListenForInput();

	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageInterrupted();

	UFUNCTION()
	void OnInputPressed(float TimeWaited);

	UFUNCTION()
	void OnComboWindowBegin();

	// bTransitioned로 중복 호출을 막고, 이전 몽타주 태스크를 정리한 뒤 다음 타를 재생한다.
	void ProceedToNextAttack();

	int32 CurrentAttackIndex = INDEX_NONE;

	// 선입력 구간 중 공격 입력이 있었는지(이어가기 구간 중 입력은 즉시 전환하므로 여기 저장할 필요가 없다).
	bool bInputBuffered = false;

	// 이미 다음 타로 전환했는지 — 전환 후 도착하는 몽타주 중단 콜백을 무시하기 위한 가드.
	bool bTransitioned = false;

	UPROPERTY()
	TObjectPtr<UAbilityTask_PlayMontageAndWait> CurrentMontageTask;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitInputPress> CurrentInputTask;
};
