#include "Combat/BlademasterTargetingComponent.h"

#include "Combat/TargetableComponent.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

UBlademasterTargetingComponent::UBlademasterTargetingComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UBlademasterTargetingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// TWeakObjectPtr는 대상이 파괴되면 조용히 무효화될 뿐 이벤트를 주지 않는다.
	// 락온 중에만(SetCurrentTarget에서 틱을 켬) 매 프레임 확인해 명시적으로 해제한다.
	if (CurrentTarget.IsStale())
	{
		SetCurrentTarget(nullptr);
	}
}

bool UBlademasterTargetingComponent::ToggleLockOn()
{
	if (IsLockedOn())
	{
		ReleaseLockOn();
		return false;
	}

	const AActor* Owner = GetOwner();
	const APlayerController* PlayerController = GetOwningPlayerController();
	if (!Owner || !PlayerController)
	{
		return false;
	}

	FVector CameraLocation;
	FRotator CameraRotation;
	PlayerController->GetPlayerViewPoint(CameraLocation, CameraRotation);
	const FVector CameraForward = CameraRotation.Vector();

	const TArray<AActor*> Candidates = GatherCandidates(Owner->GetActorLocation());
	UE_LOG(LogTemp, Log, TEXT("[Targeting] ToggleLockOn: %d candidate(s) in range"), Candidates.Num());

	AActor* BestCandidate = nullptr;
	float BestAngle = TNumericLimits<float>::Max();

	for (AActor* Candidate : Candidates)
	{
		float Angle;
		const bool bPassed = PassesAngleAndOcclusion(Candidate, CameraLocation, CameraForward, Angle);
		UE_LOG(LogTemp, Log, TEXT("[Targeting]   %s: angle=%.1f passed=%s"), *GetNameSafe(Candidate), Angle, bPassed ? TEXT("true") : TEXT("false"));

		if (!bPassed)
		{
			continue;
		}

		if (Angle < BestAngle)
		{
			BestAngle = Angle;
			BestCandidate = Candidate;
		}
	}

	if (BestCandidate)
	{
		SetCurrentTarget(BestCandidate);
		return true;
	}

	return false;
}

void UBlademasterTargetingComponent::ReleaseLockOn()
{
	SetCurrentTarget(nullptr);
}

void UBlademasterTargetingComponent::SwitchTargetLeft()
{
	SwitchTarget(-1.f);
}

void UBlademasterTargetingComponent::SwitchTargetRight()
{
	SwitchTarget(1.f);
}

void UBlademasterTargetingComponent::EvaluateMouseSwitchInput(float MouseDeltaX, float DeltaSeconds)
{
	if (!IsLockedOn() || DeltaSeconds <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const UWorld* World = GetWorld();
	const double Now = World ? World->GetTimeSeconds() : 0.0;

	if (Now - LastSwitchTargetTime < TargetSwitchCooldownSeconds)
	{
		return;
	}

	const float Speed = MouseDeltaX / DeltaSeconds;

	if (Speed >= TargetSwitchSpeedThreshold)
	{
		SwitchTargetRight();
		LastSwitchTargetTime = Now;
	}
	else if (Speed <= -TargetSwitchSpeedThreshold)
	{
		SwitchTargetLeft();
		LastSwitchTargetTime = Now;
	}
}

void UBlademasterTargetingComponent::EvaluateStickSwitchInput(float StickX)
{
	if (!IsLockedOn() || bStickSwitchConsumedThisGesture)
	{
		return;
	}

	if (StickX >= TargetSwitchStickThreshold)
	{
		UE_LOG(LogTemp, Log, TEXT("[Targeting] Stick switch RIGHT (StickX=%.2f)"), StickX);
		SwitchTargetRight();
		bStickSwitchConsumedThisGesture = true;
	}
	else if (StickX <= -TargetSwitchStickThreshold)
	{
		UE_LOG(LogTemp, Log, TEXT("[Targeting] Stick switch LEFT (StickX=%.2f)"), StickX);
		SwitchTargetLeft();
		bStickSwitchConsumedThisGesture = true;
	}
}

void UBlademasterTargetingComponent::ResetStickSwitchGesture()
{
	UE_LOG(LogTemp, Log, TEXT("[Targeting] Stick switch gesture reset"));
	bStickSwitchConsumedThisGesture = false;
}

void UBlademasterTargetingComponent::SwitchTarget(float ScreenDirection)
{
	if (!IsLockedOn())
	{
		return;
	}

	const AActor* Owner = GetOwner();
	const APlayerController* PlayerController = GetOwningPlayerController();
	if (!Owner || !PlayerController)
	{
		return;
	}

	float CurrentScreenX;
	if (!GetScreenSpaceX(CurrentTarget.Get(), CurrentScreenX))
	{
		return;
	}

	FVector CameraLocation;
	FRotator CameraRotation;
	PlayerController->GetPlayerViewPoint(CameraLocation, CameraRotation);
	const FVector CameraForward = CameraRotation.Vector();

	AActor* BestCandidate = nullptr;
	float BestDelta = TNumericLimits<float>::Max();

	for (AActor* Candidate : GatherCandidates(Owner->GetActorLocation()))
	{
		if (Candidate == CurrentTarget.Get())
		{
			continue;
		}

		float Angle;
		if (!PassesAngleAndOcclusion(Candidate, CameraLocation, CameraForward, Angle))
		{
			continue;
		}

		float CandidateScreenX;
		if (!GetScreenSpaceX(Candidate, CandidateScreenX))
		{
			continue;
		}

		// ScreenDirection과 같은 방향(왼쪽/오른쪽)에 있는 후보만 본다.
		const float Delta = (CandidateScreenX - CurrentScreenX) * ScreenDirection;
		if (Delta <= 0.f)
		{
			continue;
		}

		if (Delta < BestDelta)
		{
			BestDelta = Delta;
			BestCandidate = Candidate;
		}
	}

	if (BestCandidate)
	{
		SetCurrentTarget(BestCandidate);
	}
}

void UBlademasterTargetingComponent::SetCurrentTarget(AActor* NewTarget)
{
	// CurrentTarget.Get()은 "한 번도 설정된 적 없음"과 "설정됐던 대상이 파괴됨"
	// 둘 다 nullptr을 반환해 구분이 안 된다. 후자와 진짜 nullptr 대입을 구분하려면
	// IsExplicitlyNull()(명시적으로 null이 대입/초기화된 경우만 true)을 써야 한다.
	const bool bUnchanged = NewTarget ? CurrentTarget.Get() == NewTarget : CurrentTarget.IsExplicitlyNull();
	if (bUnchanged)
	{
		return;
	}

	CurrentTarget = NewTarget;
	SetComponentTickEnabled(NewTarget != nullptr);

	UE_LOG(LogTemp, Log, TEXT("[Targeting] %s: %s"), *GetNameSafe(GetOwner()),
		NewTarget ? *FString::Printf(TEXT("locked onto %s"), *GetNameSafe(NewTarget)) : TEXT("released"));

	OnLockOnTargetChanged.Broadcast(NewTarget);
}

TArray<AActor*> UBlademasterTargetingComponent::GatherCandidates(const FVector& OwnerLocation) const
{
	TArray<AActor*> Result;

	const AActor* Owner = GetOwner();
	const UWorld* World = GetWorld();
	if (!Owner || !World)
	{
		return Result;
	}

	const float RadiusSquared = FMath::Square(DetectionRadius);

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Candidate = *It;
		if (Candidate == Owner || !Candidate->FindComponentByClass<UTargetableComponent>())
		{
			continue;
		}

		if (FVector::DistSquared(Candidate->GetActorLocation(), OwnerLocation) > RadiusSquared)
		{
			continue;
		}

		Result.Add(Candidate);
	}

	return Result;
}

bool UBlademasterTargetingComponent::PassesAngleAndOcclusion(const AActor* Candidate, const FVector& CameraLocation, const FVector& CameraForward, float& OutAngleDegrees) const
{
	const FVector ToCandidate = (Candidate->GetActorLocation() - CameraLocation).GetSafeNormal();
	const float Dot = FMath::Clamp(FVector::DotProduct(CameraForward, ToCandidate), -1.f, 1.f);
	OutAngleDegrees = FMath::RadiansToDegrees(FMath::Acos(Dot));

	if (OutAngleDegrees > DetectionHalfAngleDegrees)
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetOwner());
	QueryParams.AddIgnoredActor(Candidate);

	FHitResult HitResult;
	const bool bBlocked = World->LineTraceSingleByChannel(HitResult, CameraLocation, Candidate->GetActorLocation(), OcclusionTraceChannel, QueryParams);

	return !bBlocked;
}

bool UBlademasterTargetingComponent::GetScreenSpaceX(const AActor* Target, float& OutScreenX) const
{
	const APlayerController* PlayerController = GetOwningPlayerController();
	if (!PlayerController || !Target)
	{
		return false;
	}

	FVector2D ScreenPosition;
	if (!PlayerController->ProjectWorldLocationToScreen(Target->GetActorLocation(), ScreenPosition))
	{
		return false;
	}

	OutScreenX = ScreenPosition.X;
	return true;
}

APlayerController* UBlademasterTargetingComponent::GetOwningPlayerController() const
{
	const APawn* OwningPawn = Cast<APawn>(GetOwner());
	return OwningPawn ? Cast<APlayerController>(OwningPawn->GetController()) : nullptr;
}
