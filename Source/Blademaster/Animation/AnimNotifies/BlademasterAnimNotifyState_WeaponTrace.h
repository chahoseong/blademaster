#pragma once

#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "CoreMinimal.h"
#include "BlademasterAnimNotifyState_WeaponTrace.generated.h"

// 구간 동안 소유 캐릭터의 UBlademasterWeaponTraceComponent에 칼 판정을 위임한다.
// 이 노티파이는 상태를 갖지 않는다 — 같은 몽타주를 여러 캐릭터가 동시에 재생할 수 있어
// 노티파이 인스턴스 자체가 공유되기 때문에, 모든 상태는 항상 대상 캐릭터의 컴포넌트에 있다.
// 판정은 서버 전용이라 권한이 없으면 즉시 반환한다(몽타주는 LocalPredicted라 클라에서도
// 재생되므로, 클라에서 판정이 도는 걸 막으려면 여기서 직접 확인해야 한다).
UCLASS()
class BLADEMASTER_API UBlademasterAnimNotifyState_WeaponTrace : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};
