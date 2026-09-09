#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "TargetableComponent.generated.h"

// 락온 후보로 취급할 액터에 붙이는 마커 컴포넌트. 별도 로직은 없다.
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class BLADEMASTER_API UTargetableComponent : public UActorComponent
{
	GENERATED_BODY()
};
