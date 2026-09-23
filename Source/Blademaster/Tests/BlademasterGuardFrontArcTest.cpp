#include "Combat/BlademasterGameplayAbility_Guard.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// 가드 유효 범위의 계약: 수평면에서 공격자가 피격자 정면 ±반각 안에 있으면 정면이다.
// 경계(정확히 반각)는 포함하고, 높이 차이는 무시하며, 피격자가 바라보는 방향을 기준으로 잰다.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBlademasterGuardFrontArcTest, "Blademaster.Combat.Guard.FrontArc",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBlademasterGuardFrontArcTest::RunTest(const FString& Parameters)
{
	const FVector VictimLocation(100.f, 200.f, 0.f);
	const FVector VictimForward(1.f, 0.f, 0.f);

	// 피격자 정면에서 수평으로 Degrees만큼 돈 방향, 300cm 떨어진 공격자 위치. 양수는 피격자의 오른쪽(+Y).
	auto AttackerAt = [&VictimLocation](float Degrees, float Height = 0.f)
	{
		const float Radians = FMath::DegreesToRadians(Degrees);
		return VictimLocation + FVector(FMath::Cos(Radians), FMath::Sin(Radians), 0.f) * 300.f + FVector(0.f, 0.f, Height);
	};

	auto IsFront = [&](const FVector& Attacker, float HalfAngle, const FVector& Forward = FVector(1.f, 0.f, 0.f))
	{
		return UBlademasterGameplayAbility_Guard::IsWithinFrontArc(VictimLocation, Forward, Attacker, HalfAngle);
	};

	// 반각 60°
	TestTrue(TEXT("반각 60: 정면 0°는 정면"), IsFront(AttackerAt(0.f), 60.f));
	TestTrue(TEXT("반각 60: 오른쪽 59°는 정면"), IsFront(AttackerAt(59.f), 60.f));
	TestTrue(TEXT("반각 60: 정확히 60°는 정면(경계 포함)"), IsFront(AttackerAt(60.f), 60.f));
	TestFalse(TEXT("반각 60: 오른쪽 61°는 비정면"), IsFront(AttackerAt(61.f), 60.f));
	TestTrue(TEXT("반각 60: 왼쪽 59°는 정면"), IsFront(AttackerAt(-59.f), 60.f));
	TestFalse(TEXT("반각 60: 왼쪽 61°는 비정면"), IsFront(AttackerAt(-61.f), 60.f));
	TestFalse(TEXT("반각 60: 측면 90°는 비정면"), IsFront(AttackerAt(90.f), 60.f));
	TestFalse(TEXT("반각 60: 배후 180°는 비정면"), IsFront(AttackerAt(180.f), 60.f));

	// 경계는 반각을 따라간다
	TestTrue(TEXT("반각 30: 29°는 정면"), IsFront(AttackerAt(29.f), 30.f));
	TestFalse(TEXT("반각 30: 31°는 비정면"), IsFront(AttackerAt(31.f), 30.f));

	// 높이 차이는 무시한다 — 3D 각도로 재면 결과가 뒤집히는 높이를 쓴다.
	TestTrue(TEXT("높이 차이 무시: 59°에서 1000cm 위에 있어도 정면"), IsFront(AttackerAt(59.f, 1000.f), 60.f));
	TestTrue(TEXT("높이 차이 무시: 피격자 정면이 위로 60° 기울어 있어도 수평 방향으로 잰다"),
		IsFront(AttackerAt(59.f), 60.f, FVector(0.5f, 0.f, 0.866f)));

	// 피격자가 바라보는 방향을 기준으로 잰다
	const FVector FacingRight(0.f, 1.f, 0.f);
	TestTrue(TEXT("피격자가 +Y를 보면 +Y의 공격자는 정면"), IsFront(AttackerAt(90.f), 60.f, FacingRight));
	TestFalse(TEXT("피격자가 +Y를 보면 +X의 공격자는 비정면"), IsFront(AttackerAt(0.f), 60.f, FacingRight));

	TestTrue(TEXT("위치가 겹쳐 방향이 없으면 정면"), IsFront(VictimLocation, 60.f));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
