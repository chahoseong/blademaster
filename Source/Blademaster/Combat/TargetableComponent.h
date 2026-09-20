#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "TargetableComponent.generated.h"

// 락온 후보로 취급할 액터에 붙이는 마커 컴포넌트.
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class BLADEMASTER_API UTargetableComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// 지금 락온 후보로 유효한지. 소유 액터가 쓰러진 상태(State.Death)면 false.
	bool IsTargetable() const;
};
