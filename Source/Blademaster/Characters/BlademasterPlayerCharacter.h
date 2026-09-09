// Copyright Epic Games, Inc. All Rights Reserved.

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

protected:
	virtual void BeginPlay() override;
	virtual void NotifyControllerChanged() override;

	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void LockOn(const FInputActionValue& Value);
	void SwitchTargetMouse(const FInputActionValue& Value);
	void SwitchTargetStick(const FInputActionValue& Value);
	void ResetSwitchTargetStick(const FInputActionValue& Value);

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
};
