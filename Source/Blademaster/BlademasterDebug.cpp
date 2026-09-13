#include "BlademasterDebug.h"

#if !UE_BUILD_SHIPPING

#include "DrawDebugHelpers.h"
#include "GameFramework/Actor.h"
#include "HAL/IConsoleManager.h"

namespace BlademasterDebug
{
	static TAutoConsoleVariable<bool> CVarDebugCombat(
		TEXT("Blademaster.Debug.Combat"),
		false,
		TEXT("켜면 ASC를 가진 캐릭터 머리 위에 체력, 자세, 콤보, 구간 등 전투 상태를 그린다."),
		ECVF_Cheat);

	bool IsCombatDebugEnabled()
	{
		return CVarDebugCombat.GetValueOnGameThread();
	}

	void DrawDebugTextLine(const AActor* Actor, int32 LineIndex, const FString& Text, const FColor& Color)
	{
		if (!Actor)
		{
			return;
		}

		constexpr float BaseHeightAboveRoot = 100.f;
		constexpr float LineSpacing = 20.f;

		const FVector Offset(0.f, 0.f, BaseHeightAboveRoot + LineIndex * LineSpacing);
		DrawDebugString(Actor->GetWorld(), Offset, Text, const_cast<AActor*>(Actor), Color, 0.f, false);
	}
}

#endif // !UE_BUILD_SHIPPING
