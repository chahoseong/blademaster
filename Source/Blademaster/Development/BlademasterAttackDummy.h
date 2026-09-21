#pragma once

#include "AbilitySystemInterface.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BlademasterAttackDummy.generated.h"

class UAbilitySystemComponent;
class UBlademasterAttributeSet;
class UGameplayEffect;

// 가드·패링 검증용 공격 장치. 레벨에 배치해 쓰며 전투 캐릭터의 구성에 포함하지 않는다.
//
// 라우터(UBlademasterCombatComponent)가 공격자에게 요구하는 것만 갖는다 — IAbilitySystemInterface로
// ASC를 돌려주고(데미지 GE 스펙을 이 ASC로 만든다), 패링 시 깎일 UBlademasterAttributeSet을 갖는다.
// ASC는 InitializeComponent에서 이 액터를 Owner·Avatar로 초기화하고, 이 액터의 AttributeSet
// 서브오브젝트를 스스로 등록한다.
UCLASS()
class BLADEMASTER_API ABlademasterAttackDummy : public AActor, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ABlademasterAttackDummy();

	virtual void Tick(float DeltaSeconds) override;

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "AbilitySystem")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, Category = "AbilitySystem")
	TObjectPtr<UBlademasterAttributeSet> AttributeSet;

	// BeginPlay에 한 번 적용해 체력·자세를 초기화한다. 캐릭터가 쓰는 초기화 GE 에셋을 그대로 지정한다.
	UPROPERTY(EditAnywhere, Category = "AttackDummy")
	TSubclassOf<UGameplayEffect> InitializeAttributesEffect;

#if !UE_BUILD_SHIPPING
private:
	// ASC에 등록된 AttributeSet에서 체력·자세를 읽어 장치 위에 그린다.
	void DrawAttributeText() const;
#endif
};
