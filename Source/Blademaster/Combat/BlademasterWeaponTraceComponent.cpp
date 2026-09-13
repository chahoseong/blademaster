#include "Combat/BlademasterWeaponTraceComponent.h"

#include "AbilitySystemInterface.h"
#include "BlademasterCollisionChannels.h"
#include "BlademasterLogChannels.h"
#include "Characters/BlademasterCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"

#if !UE_BUILD_SHIPPING
#include "BlademasterDebug.h"
#include "DrawDebugHelpers.h"
#endif

namespace
{
	const FName BladeTipSocketName(TEXT("BladeTip"));

	// 가림 확인용 얇은 구 트레이스 반지름. 라인(반지름 0)으로 하면 얇은 벽 모서리 틈으로
	// 새어나가 오탐할 수 있어서, 칼 판정과 같은 이유로 반지름을 살짝 준다.
	constexpr float OcclusionTraceRadius = 1.f;
}

UBlademasterWeaponTraceComponent::UBlademasterWeaponTraceComponent()
{
	OcclusionObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldStatic));
	OcclusionObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldDynamic));
}

void UBlademasterWeaponTraceComponent::BeginTrace()
{
	HitActorsThisWindow.Reset();
	bWaitingForFirstFrame = true;
}

void UBlademasterWeaponTraceComponent::TickTrace()
{
	FBladePose CurrentPose;
	FVector LocalTipOffset;
	if (!GetCurrentBladePose(CurrentPose, LocalTipOffset))
	{
		return;
	}

	if (bWaitingForFirstFrame)
	{
		// 이전 위치가 없다 — 제로 길이 스윕(=오버랩 검사)으로 지금 자리에 겹친 대상을 맞은 것으로 본다.
		TArray<FVector> CurrentPoints;
		ComputeBladePoints(CurrentPose, LocalTipOffset, CurrentPoints);
		SweepPoints(CurrentPoints, CurrentPoints);

		PreviousPose = CurrentPose;
		PreviousLocalTipOffset = LocalTipOffset;
		bWaitingForFirstFrame = false;
		return;
	}

	const float AngleDeltaDegrees = FMath::RadiansToDegrees(PreviousPose.BaseRotation.AngularDistance(CurrentPose.BaseRotation));
	const int32 NumSubsteps = (AngleDeltaDegrees > SubstepAngleThresholdDegrees)
		? FMath::Clamp(FMath::CeilToInt(AngleDeltaDegrees / SubstepAngleThresholdDegrees), 1, MaxSubsteps)
		: 1;

	TArray<FVector> PreviousPoints;
	ComputeBladePoints(PreviousPose, PreviousLocalTipOffset, PreviousPoints);

	for (int32 Step = 1; Step <= NumSubsteps; ++Step)
	{
		const float Alpha = static_cast<float>(Step) / static_cast<float>(NumSubsteps);

		FBladePose SubstepPose;
		SubstepPose.BaseLocation = FMath::Lerp(PreviousPose.BaseLocation, CurrentPose.BaseLocation, Alpha);
		SubstepPose.BaseRotation = FQuat::Slerp(PreviousPose.BaseRotation, CurrentPose.BaseRotation, Alpha);

		const FVector SubstepLocalTipOffset = FMath::Lerp(PreviousLocalTipOffset, LocalTipOffset, Alpha);

		TArray<FVector> SubstepPoints;
		ComputeBladePoints(SubstepPose, SubstepLocalTipOffset, SubstepPoints);

		SweepPoints(PreviousPoints, SubstepPoints);

		PreviousPoints = SubstepPoints;
	}

	PreviousPose = CurrentPose;
	PreviousLocalTipOffset = LocalTipOffset;
}

void UBlademasterWeaponTraceComponent::EndTrace()
{
	bWaitingForFirstFrame = false;
}

bool UBlademasterWeaponTraceComponent::GetCurrentBladePose(FBladePose& OutPose, FVector& OutLocalTipOffset) const
{
	const ABlademasterCharacter* Character = Cast<ABlademasterCharacter>(GetOwner());
	const UStaticMeshComponent* Weapon = Character ? Character->GetWeaponMesh() : nullptr;
	if (!Weapon || !Weapon->DoesSocketExist(BladeTipSocketName))
	{
		return false;
	}

	// 판정 범위의 안쪽 기준점은 칼 밑동이 아니라, 무기가 실제로 붙어있는 손 소켓이다.
	// 칼 밑동만 기준으로 하면, 밑동보다 몸에 더 가까이 있는 대상은 스윕 궤적(회전 중심에서
	// 밑동 거리 이상만 도는 도넛 모양)의 안쪽 사각지대에 들어가 원리적으로 절대 맞지 않는다.
	// 액션 게임들이 흔히 판정 범위를 손 쪽까지 넉넉히 잡아 이 사각지대를 줄이는 것과 같은 이유다.
	const USkeletalMeshComponent* CharacterMesh = Character->GetMesh();
	const FName HandSocketName = Weapon->GetAttachSocketName();
	if (!CharacterMesh || !CharacterMesh->DoesSocketExist(HandSocketName))
	{
		return false;
	}

	const FTransform BaseTransform = CharacterMesh->GetSocketTransform(HandSocketName);
	const FVector TipLocation = Weapon->GetSocketTransform(BladeTipSocketName).GetLocation();

	OutPose.BaseLocation = BaseTransform.GetLocation();
	OutPose.BaseRotation = BaseTransform.GetRotation();
	OutLocalTipOffset = BaseTransform.InverseTransformPosition(TipLocation);
	return true;
}

void UBlademasterWeaponTraceComponent::ComputeBladePoints(const FBladePose& Pose, const FVector& LocalTipOffset, TArray<FVector>& OutPoints) const
{
	OutPoints.Reset();

	const ABlademasterCharacter* Character = Cast<ABlademasterCharacter>(GetOwner());
	const float Radius = Character ? Character->GetWeaponTraceRadius() : 5.f;

	const FVector TipLocation = Pose.BaseLocation + Pose.BaseRotation.RotateVector(LocalTipOffset);
	const float BladeLength = LocalTipOffset.Size();
	const float Spacing = FMath::Max(2.f * Radius, KINDA_SMALL_NUMBER);
	const int32 NumPoints = FMath::Max(2, FMath::CeilToInt(BladeLength / Spacing) + 1);

	for (int32 Index = 0; Index < NumPoints; ++Index)
	{
		const float Alpha = static_cast<float>(Index) / static_cast<float>(NumPoints - 1);
		OutPoints.Add(FMath::Lerp(Pose.BaseLocation, TipLocation, Alpha));
	}
}

void UBlademasterWeaponTraceComponent::SweepPoints(const TArray<FVector>& PreviousPoints, const TArray<FVector>& CurrentPoints)
{
	ABlademasterCharacter* Character = Cast<ABlademasterCharacter>(GetOwner());
	UWorld* World = GetWorld();
	if (!Character || !World)
	{
		return;
	}

	const float Radius = Character->GetWeaponTraceRadius();
	const FVector BladeBaseLocation = CurrentPoints.Num() > 0 ? CurrentPoints[0] : FVector::ZeroVector;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Character);

	const int32 NumPoints = FMath::Min(PreviousPoints.Num(), CurrentPoints.Num());
	for (int32 Index = 0; Index < NumPoints; ++Index)
	{
		TArray<FHitResult> HitResults;
		World->SweepMultiByChannel(HitResults, PreviousPoints[Index], CurrentPoints[Index], FQuat::Identity,
			BlademasterCollisionChannels::Weapon, FCollisionShape::MakeSphere(Radius), QueryParams);

#if !UE_BUILD_SHIPPING
		if (BlademasterDebug::IsHitboxDebugEnabled())
		{
			DrawDebugSphere(World, CurrentPoints[Index], Radius, 8, FColor::Cyan, false, 1.5f);
			DrawDebugLine(World, PreviousPoints[Index], CurrentPoints[Index], FColor::Cyan, false, 1.5f, 0, 2.f);
		}
#endif

		for (const FHitResult& Hit : HitResults)
		{
			ProcessCandidateHit(Hit, BladeBaseLocation);
		}
	}
}

void UBlademasterWeaponTraceComponent::ProcessCandidateHit(const FHitResult& Hit, const FVector& BladeBaseLocation)
{
	AActor* HitActor = Hit.GetActor();
	if (!HitActor || HitActorsThisWindow.Contains(HitActor))
	{
		return;
	}

	const IAbilitySystemInterface* AbilitySystemInterface = Cast<IAbilitySystemInterface>(HitActor);
	if (!AbilitySystemInterface || !AbilitySystemInterface->GetAbilitySystemComponent())
	{
		return;
	}

	const FVector HitLocation = Hit.ImpactPoint;
	const bool bOccluded = IsOccluded(BladeBaseLocation, HitLocation);

	UE_LOG(LogBlademasterCombat, Verbose, TEXT("%s: 가림 확인 %s->%s = %s"), *GetNameSafe(GetOwner()),
		*BladeBaseLocation.ToCompactString(), *HitLocation.ToCompactString(), bOccluded ? TEXT("가려짐") : TEXT("안 가려짐"));

#if !UE_BUILD_SHIPPING
	if (BlademasterDebug::IsHitboxDebugEnabled())
	{
		// 가림 확인 트레이스(칼 밑동 -> 맞은 지점) 자체를 그린다 — 막혔으면 주황, 안 막혔으면 초록.
		// 얇은 선은 눈에 잘 안 띄어서 두껍게 그린다.
		DrawDebugLine(GetWorld(), BladeBaseLocation, HitLocation, bOccluded ? FColor::Orange : FColor::Green, false, 1.5f, 0, 3.f);
	}
#endif

	if (bOccluded)
	{
#if !UE_BUILD_SHIPPING
		if (BlademasterDebug::IsHitboxDebugEnabled())
		{
			DrawDebugSphere(GetWorld(), HitLocation, 6.f, 8, FColor::Silver, false, 1.5f);
		}
#endif
		return;
	}

	HitActorsThisWindow.Add(HitActor);

	UE_LOG(LogBlademasterCombat, Log, TEXT("%s: %s의 %s 부위를 맞혔다"), *GetNameSafe(GetOwner()), *GetNameSafe(HitActor), *Hit.BoneName.ToString());

#if !UE_BUILD_SHIPPING
	if (BlademasterDebug::IsHitboxDebugEnabled())
	{
		DrawDebugSphere(GetWorld(), HitLocation, 8.f, 12, FColor::Red, false, 1.5f);
	}
#endif

	OnWeaponHit.Broadcast(Hit);
}

bool UBlademasterWeaponTraceComponent::IsOccluded(const FVector& From, const FVector& To) const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	const FCollisionObjectQueryParams ObjectQueryParams(OcclusionObjectTypes);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetOwner());

	FHitResult HitResult;
	return World->SweepSingleByObjectType(HitResult, From, To, FQuat::Identity, ObjectQueryParams,
		FCollisionShape::MakeSphere(OcclusionTraceRadius), QueryParams);
}
