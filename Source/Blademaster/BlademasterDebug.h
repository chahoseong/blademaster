#pragma once

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

class AActor;

namespace BlademasterDebug
{
	// Blademaster.Debug.Combat 콘솔 변수가 켜져 있는지.
	BLADEMASTER_API bool IsCombatDebugEnabled();

	// Blademaster.Debug.Hitbox 콘솔 변수가 켜져 있는지.
	BLADEMASTER_API bool IsHitboxDebugEnabled();

	// 액터 머리 위 LineIndex번째 줄(클수록 위)에 텍스트 한 줄을 그린다.
	BLADEMASTER_API void DrawDebugTextLine(const AActor* Actor, int32 LineIndex, const FString& Text, const FColor& Color = FColor::White);
}

#endif // !UE_BUILD_SHIPPING
