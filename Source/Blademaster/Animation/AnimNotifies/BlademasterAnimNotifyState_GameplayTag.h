#pragma once

#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "BlademasterAnimNotifyState_GameplayTag.generated.h"

// 구간 동안 소유자 ASC에 태그를 붙였다 뗀다. 그 외 로직은 없다 — 태그가 무엇을 의미하는지는
// 그 태그를 읽는 쪽(어빌리티 등)이 정한다.
UCLASS()
class BLADEMASTER_API UBlademasterAnimNotifyState_GameplayTag : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	// 노티파이 트랙에 클래스 이름 대신 지정한 태그를 표시한다.
	virtual FString GetNotifyName_Implementation() const override;

protected:
	UPROPERTY(EditAnywhere, Category = "Blademaster")
	FGameplayTag Tag;
};
