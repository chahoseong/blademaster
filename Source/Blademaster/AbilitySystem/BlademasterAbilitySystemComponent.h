#pragma once

#include "AbilitySystemComponent.h"
#include "CoreMinimal.h"
#include "BlademasterAbilitySystemComponent.generated.h"

// 엔진 기본 UAbilitySystemComponent는 활성 중인 어빌리티에 입력 누름/뗌을 전달만 하고,
// UAbilityTask_WaitInputPress/WaitInputRelease가 실제로 반응하는 데 필요한
// InvokeReplicatedEvent 호출은 하지 않는다(프로젝트가 직접 연결해야 하는 훅이다).
// 이 클래스가 그 연결을 담당한다.
UCLASS()
class BLADEMASTER_API UBlademasterAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	virtual void AbilitySpecInputPressed(FGameplayAbilitySpec& Spec) override;
	virtual void AbilitySpecInputReleased(FGameplayAbilitySpec& Spec) override;
};
