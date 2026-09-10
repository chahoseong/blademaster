#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "BlademasterTargetingComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnLockOnTargetChanged, AActor* /*NewTarget*/);

class APlayerController;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class BLADEMASTER_API UBlademasterTargetingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBlademasterTargetingComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// 락온 중이면 해제, 아니면 후보 중 하나를 선택해 락온한다. 락온에 성공하면 true.
	bool ToggleLockOn();

	void ReleaseLockOn();

	void SwitchTargetLeft();
	void SwitchTargetRight();

	// 마우스 X 이동 속도(픽셀/초)가 임계값을 넘으면 좌우 전환을 실행한다.
	// DeltaSeconds로 나눠 속도로 판정하는 이유: 프레임레이트에 따라 프레임당
	// delta 크기가 달라지므로, 프레임레이트 독립적인 판정을 위해 속도를 쓴다.
	void EvaluateMouseSwitchInput(float MouseDeltaX, float DeltaSeconds);

	// 게임패드 오른쪽 스틱 X(절대 위치, [-1, 1])로 좌우 전환을 판정한다.
	// 마우스와 달리 스틱은 delta가 아니라 위치라서 속도 계산이 아니라
	// 히스테리시스(임계값을 넘으면 전환, 중심 근처로 돌아와야 재무장)로 판정한다.
	void EvaluateStickSwitchInput(float StickX);

	// 스틱이 중심 근처로 돌아왔을 때(EnhancedInput Completed) 호출 — 재전환 가능하게 무장한다.
	void ResetStickSwitchGesture();

	bool IsLockedOn() const { return CurrentTarget.IsValid(); }
	AActor* GetCurrentTarget() const { return CurrentTarget.Get(); }

	// 락온 대상이 바뀔 때마다 호출된다(락온/해제/전환 전부 포함, 해제 시 NewTarget = nullptr).
	FOnLockOnTargetChanged OnLockOnTargetChanged;

protected:
	// 후보 탐지 반경. 컴포넌트 소유 액터 위치 기준(cm).
	UPROPERTY(EditAnywhere, Category = "Targeting")
	float DetectionRadius = 1500.f;

	// 카메라 정면 기준 후보로 인정하는 각도(편측, degree).
	UPROPERTY(EditAnywhere, Category = "Targeting")
	float DetectionHalfAngleDegrees = 60.f;

	UPROPERTY(EditAnywhere, Category = "Targeting")
	TEnumAsByte<ECollisionChannel> OcclusionTraceChannel = ECC_Visibility;

	// 좌우 전환 플릭으로 인정할 마우스 X 이동 속도(픽셀/초).
	UPROPERTY(EditAnywhere, Category = "Targeting")
	float TargetSwitchSpeedThreshold = 100.f;

	// 전환 후 다음 전환까지의 최소 대기 시간(초). 마우스 전환에만 적용된다.
	UPROPERTY(EditAnywhere, Category = "Targeting")
	float TargetSwitchCooldownSeconds = 0.2f;

	// 스틱 좌우 전환으로 인정할 위치 임계값([-1, 1] 기준).
	UPROPERTY(EditAnywhere, Category = "Targeting")
	float TargetSwitchStickThreshold = 0.6f;

private:
	void SwitchTarget(float ScreenDirection);
	void SetCurrentTarget(AActor* NewTarget);

	TArray<AActor*> GatherCandidates(const FVector& OwnerLocation) const;
	bool PassesAngleAndOcclusion(const AActor* Candidate, const FVector& CameraLocation, const FVector& CameraForward, float& OutAngleDegrees) const;
	bool GetScreenSpaceX(const AActor* Target, float& OutScreenX) const;

	APlayerController* GetOwningPlayerController() const;

	TWeakObjectPtr<AActor> CurrentTarget;

	double LastSwitchTargetTime = -1.0;
	bool bStickSwitchConsumedThisGesture = false;
};
