#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "BlademasterWeaponTraceComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnWeaponTraceHit, const FHitResult&);

// 장착 무기(칼)의 궤적을 스윕해서 순수 물리 판정(누구를, 어디를, 어느 부위를, 어느 방향으로
// 맞혔는지)만 한다. 어떤 공격인지는 모른다 — 그건 이미 알고 있는 콤보 어빌리티가
// 이 컴포넌트의 결과에 덧붙인다. Begin/Tick/EndTrace는 몽타주의 AnimNotifyState가 직접 호출한다.
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class BLADEMASTER_API UBlademasterWeaponTraceComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBlademasterWeaponTraceComponent();

	// 판정 구간 시작. 맞은 대상 목록을 비우고, 다음 TickTrace가 첫 프레임(이전 위치 없음)임을 기록한다.
	void BeginTrace();

	// 판정 구간 동안 애니메이션이 갱신될 때마다 호출된다. 이전 틱과 비교해 substep을 나누고,
	// 각 점을 구 스윕해 판정한다.
	void TickTrace();

	// 판정 구간 종료. 지금은 상태 정리 외에 별다른 일을 하지 않는다.
	void EndTrace();

	// 순수 물리 판정 결과. 대상(GetActor)·지점(Location)·부위(BoneName)는 FHitResult에 이미 있고,
	// 칼이 움직이던 방향은 TraceEnd - TraceStart로 구할 수 있다.
	FOnWeaponTraceHit OnWeaponHit;

protected:
	// 이번 프레임에 칼이 이 각도(도) 이상 돌았으면 substep으로 나눈다.
	UPROPERTY(EditDefaultsOnly, Category = "WeaponTrace")
	float SubstepAngleThresholdDegrees = 15.f;

	// substep 최대 분할 수.
	UPROPERTY(EditDefaultsOnly, Category = "WeaponTrace")
	int32 MaxSubsteps = 8;

	// 가림 확인(칼 밑동 → 맞은 지점)에서 "막았다"고 볼 오브젝트 타입.
	UPROPERTY(EditDefaultsOnly, Category = "WeaponTrace")
	TArray<TEnumAsByte<EObjectTypeQuery>> OcclusionObjectTypes;

private:
	struct FBladePose
	{
		FVector BaseLocation = FVector::ZeroVector;
		FQuat BaseRotation = FQuat::Identity;
	};

	// 지금 소켓 트랜스폼으로부터 칼 자세와, 밑동 기준 칼끝의 로컬 오프셋을 읽어온다.
	bool GetCurrentBladePose(FBladePose& OutPose, FVector& OutLocalTipOffset) const;

	// 칼 자세 + 로컬 팁 오프셋으로부터, 밑동~칼끝을 구 지름 이하 간격으로 나눈 점들의 월드 위치를 만든다.
	void ComputeBladePoints(const FBladePose& Pose, const FVector& LocalTipOffset, TArray<FVector>& OutPoints) const;

	// 점마다 PreviousPoints[i] -> CurrentPoints[i]로 구 스윕해서 판정한다.
	void SweepPoints(const TArray<FVector>& PreviousPoints, const TArray<FVector>& CurrentPoints);

	// 대상 하나가 맞았을 때: 자기 자신·ASC 없음·이미 맞음·가림을 걸러내고, 통과하면 결과를 낸다.
	void ProcessCandidateHit(const FHitResult& Hit, const FVector& BladeBaseLocation);

	bool IsOccluded(const FVector& From, const FVector& To) const;

	bool bWaitingForFirstFrame = false;

	FBladePose PreviousPose;
	FVector PreviousLocalTipOffset = FVector::ZeroVector;

	TArray<TWeakObjectPtr<AActor>> HitActorsThisWindow;
};
