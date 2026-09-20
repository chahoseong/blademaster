#include "Combat/BlademasterCombatComponent.h"
#include "Engine/HitResult.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// 피격 방향 판정의 계약: 칼이 피격자의 오른쪽으로 지나가면 Left(공격자가 왼쪽에서 왔다),
// 왼쪽으로 지나가면 Right이고, 좌우 성분이 임계값(0.3) 미만이면 Front다.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBlademasterHitDirectionTest, "Blademaster.Combat.HitDirection",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBlademasterHitDirectionTest::RunTest(const FString& Parameters)
{
	// 회전하지 않은 피격자: 앞이 +X, 오른쪽이 +Y.
	const FVector VictimRight(0.f, 1.f, 0.f);

	auto Compute = [](const FVector& Swing, const FVector& Right)
	{
		FHitResult Hit;
		Hit.TraceStart = FVector(100.f, 200.f, 50.f);
		Hit.TraceEnd = Hit.TraceStart + Swing * 80.f;
		return UBlademasterCombatComponent::ComputeHitDirection(Hit, Right);
	};

	// 단위 벡터 중 피격자 오른쪽 성분이 Dot인 것.
	auto SwingWithRightDot = [](float Dot)
	{
		return FVector(FMath::Sqrt(1.f - Dot * Dot), Dot, 0.f);
	};

	using EDir = EBlademasterHitDirection;

	TestEqual(TEXT("칼이 피격자 오른쪽으로 지나가면 Left"), Compute(FVector(0.f, 1.f, 0.f), VictimRight), EDir::Left);
	TestEqual(TEXT("칼이 피격자 왼쪽으로 지나가면 Right"), Compute(FVector(0.f, -1.f, 0.f), VictimRight), EDir::Right);
	TestEqual(TEXT("칼이 앞뒤로만 움직이면 Front"), Compute(FVector(1.f, 0.f, 0.f), VictimRight), EDir::Front);

	TestEqual(TEXT("오른쪽 성분 0.29는 Front"), Compute(SwingWithRightDot(0.29f), VictimRight), EDir::Front);
	TestEqual(TEXT("오른쪽 성분 0.31은 Left"), Compute(SwingWithRightDot(0.31f), VictimRight), EDir::Left);
	TestEqual(TEXT("왼쪽 성분 0.29는 Front"), Compute(SwingWithRightDot(-0.29f), VictimRight), EDir::Front);
	TestEqual(TEXT("왼쪽 성분 0.31은 Right"), Compute(SwingWithRightDot(-0.31f), VictimRight), EDir::Right);

	// 피격자가 돌아 오른쪽이 +X가 되면, 같은 월드 방향도 피격자 기준으로 다른 결과가 된다.
	const FVector RotatedVictimRight(1.f, 0.f, 0.f);
	TestEqual(TEXT("피격자가 회전하면 피격자 기준으로 판단: +X 스윙은 Left"), Compute(FVector(1.f, 0.f, 0.f), RotatedVictimRight), EDir::Left);
	TestEqual(TEXT("피격자가 회전하면 피격자 기준으로 판단: +Y 스윙은 Front"), Compute(FVector(0.f, 1.f, 0.f), RotatedVictimRight), EDir::Front);

	FHitResult ZeroSwing;
	ZeroSwing.TraceStart = FVector(1.f, 2.f, 3.f);
	ZeroSwing.TraceEnd = ZeroSwing.TraceStart;
	TestEqual(TEXT("스윕 길이가 0이면 Front"), UBlademasterCombatComponent::ComputeHitDirection(ZeroSwing, VictimRight), EDir::Front);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
