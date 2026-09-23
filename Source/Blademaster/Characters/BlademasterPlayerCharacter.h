#pragma once

#include "CoreMinimal.h"
#include "Characters/BlademasterCharacter.h"
#include "BlademasterPlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
class UBlademasterTargetingComponent;
struct FInputActionValue;

UCLASS()
class BLADEMASTER_API ABlademasterPlayerCharacter : public ABlademasterCharacter
{
	GENERATED_BODY()

public:
	ABlademasterPlayerCharacter();

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void Tick(float DeltaSeconds) override;

	// 락온 중이면 대상 방향, 아니면 마지막 이동 입력 방향(입력이 없으면 현재 회전 유지).
	virtual FRotator GetAttackDirection() const override;

protected:
	virtual void BeginPlay() override;
	virtual void NotifyControllerChanged() override;

	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void LockOn(const FInputActionValue& Value);
	void SwitchTargetMouse(const FInputActionValue& Value);
	void SwitchTargetStick(const FInputActionValue& Value);
	void ResetSwitchTargetStick(const FInputActionValue& Value);
	void AttackPressed(const FInputActionValue& Value);
	void AttackReleased(const FInputActionValue& Value);
	void GuardPressed(const FInputActionValue& Value);
	void GuardReleased(const FInputActionValue& Value);

	// 락온 대상이 바뀔 때(락온/해제 포함) 회전 모드를 전환한다.
	void OnLockOnTargetChanged(AActor* NewTarget);

	// 락온 중 캐릭터가 대상을 향하도록 ControlRotation을 보간하는 속도.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Targeting", meta = (AllowPrivateAccess = "true"))
	float LockOnRotationInterpSpeed = 10.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Targeting", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBlademasterTargetingComponent> TargetingComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> LockOnAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> SwitchTargetMouseAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> SwitchTargetStickAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> AttackAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> GuardAction;

private:
	// 락온 중이 아닐 때, 콤보 전체에 걸쳐 스틱 방향을 한 번만 반영하기 위한 캐시.
	// State.Attacking이 없는 동안(Tick에서) 매번 리셋되어, 다음 공격 때 새로 잡힌다.
	mutable bool bFreeAimDirectionCached = false;
	mutable FRotator CachedFreeAimDirection = FRotator::ZeroRotator;
};
