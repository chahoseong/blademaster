#pragma once

#include "AbilitySystemInterface.h"
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BlademasterCharacter.generated.h"

class UAbilitySystemComponent;
class UBlademasterAttributeSet;
class UBlademasterCombatComponent;
class UBlademasterWeaponTraceComponent;
class UGameplayEffect;
class UMotionWarpingComponent;
class UStaticMeshComponent;

UCLASS()
class BLADEMASTER_API ABlademasterCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ABlademasterCharacter();

	virtual void Tick(float DeltaSeconds) override;

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UStaticMeshComponent* GetWeaponMesh() const { return WeaponMesh; }
	UStaticMeshComponent* GetShieldMesh() const { return ShieldMesh; }
	UBlademasterCombatComponent* GetCombatComponent() const { return CombatComponent; }
	UMotionWarpingComponent* GetMotionWarpingComponent() const { return MotionWarpingComponent; }
	UBlademasterWeaponTraceComponent* GetWeaponTraceComponent() const { return WeaponTraceComponent; }

	float GetWeaponTraceRadius() const { return WeaponTraceRadius; }

	// 공격이 향해야 할 방향. 어빌리티는 입력을 직접 읽지 않고 이 함수로 묻는다.
	// 기본값은 현재 액터 회전(돌지 않음) — 플레이어가 재정의한다.
	virtual FRotator GetAttackDirection() const;

#if !UE_BUILD_SHIPPING
	// 매 틱, "지금 네 디버그 정보를 그려라"라는 신호. 이 캐릭터가 소유한 어빌리티 등이
	// 여기에 바인딩해서 자기 정보를 그린다 — 캐릭터는 누가 듣는지 알 필요가 없다.
	FSimpleMulticastDelegate OnDrawDebug;
#endif

protected:
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AbilitySystem", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AbilitySystem", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBlademasterAttributeSet> AttributeSet;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AbilitySystem", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UGameplayEffect> InitializeAttributesEffect;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> WeaponMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> ShieldMesh;

	// 칼 두께(판정용 스윕 구 반지름), cm.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon", meta = (AllowPrivateAccess = "true"))
	float WeaponTraceRadius = 5.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBlademasterCombatComponent> CombatComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMotionWarpingComponent> MotionWarpingComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBlademasterWeaponTraceComponent> WeaponTraceComponent;

#if !UE_BUILD_SHIPPING
private:
	// 체력·자세·구간 태그 등 이 캐릭터 자신이 소유한 정보를 머리 위에 그린다.
	void DrawOwnCombatDebugText() const;
#endif
};
