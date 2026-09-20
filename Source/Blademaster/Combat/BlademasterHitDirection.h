#pragma once

#include "CoreMinimal.h"
#include "BlademasterHitDirection.generated.h"

// 판정이 넘겨준 칼의 이동 방향을 피격자 기준 좌/우/정면으로 바꾼 값. 피격·사망 몽타주 선택에 쓰인다.
UENUM(BlueprintType)
enum class EBlademasterHitDirection : uint8
{
	Front,
	Left,
	Right
};
