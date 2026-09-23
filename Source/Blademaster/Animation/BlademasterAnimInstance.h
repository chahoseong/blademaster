#pragma once

#include "Animation/AnimInstance.h"
#include "CoreMinimal.h"
#include "BlademasterAnimInstance.generated.h"

class ACharacter;

UCLASS()
class BLADEMASTER_API UBlademasterAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

protected:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	// MaxSpeed로 정규화된 [-1, 1] 값. 실제 cm/s 속도가 아니다.
	UPROPERTY(BlueprintReadOnly, Category = "Movement", meta = (AllowPrivateAccess = "true"))
	float MoveForward = 0.f;

	// MaxSpeed로 정규화된 [-1, 1] 값. 실제 cm/s 속도가 아니다.
	UPROPERTY(BlueprintReadOnly, Category = "Movement", meta = (AllowPrivateAccess = "true"))
	float MoveRight = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Look", meta = (AllowPrivateAccess = "true"))
	float HorizontalAngle = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Look", meta = (AllowPrivateAccess = "true"))
	float VerticalAngle = 0.f;

	// State.Guard를 보유하는 동안 true. 상체 가드 자세의 블렌드에 쓴다.
	UPROPERTY(BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	bool bIsGuarding = false;

private:
	UPROPERTY()
	TObjectPtr<ACharacter> OwningCharacter;
};
