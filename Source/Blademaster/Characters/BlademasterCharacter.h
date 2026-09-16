#pragma once

#include "AbilitySystemInterface.h"
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BlademasterCharacter.generated.h"

class UAbilitySystemComponent;
class UAnimMontage;
class UBlademasterAttributeSet;
class UBlademasterCombatComponent;
class UBlademasterWeaponTraceComponent;
class UGameplayEffect;
class UMotionWarpingComponent;
class UStaticMeshComponent;

// 판정이 넘겨준 칼의 이동 방향을 피격자 기준 좌/우/정면으로 바꾼 값. 피격·사망 몽타주 선택에 쓰인다.
UENUM(BlueprintType)
enum class EBlademasterHitDirection : uint8
{
	Front,
	Left,
	Right
};

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

	// 방향에 맞는 피격 몽타주 후보 목록(왼쪽만 2개 — 랜덤 선택용, 나머지는 1개). 피격 어빌리티가 쓴다.
	const TArray<TObjectPtr<UAnimMontage>>& GetHitReactMontages(EBlademasterHitDirection Direction) const;

	// 방향에 맞는 사망 몽타주. 정면은 왼쪽 몽타주를 대신 쓴다(둘 중 하나만 있으면 되는 요구사항).
	UAnimMontage* GetDeathMontage(EBlademasterHitDirection Direction) const;

	// 부활 시 체력·자세를 되돌리는 데 재사용한다(GA_Respawn).
	TSubclassOf<UGameplayEffect> GetInitializeAttributesEffect() const { return InitializeAttributesEffect; }

	// 죽을 때 꺼서(GA_Hit) 무기 판정에서 빠지게 하고, 부활할 때 다시 켠다(GA_Respawn).
	// 메시의 Weapon 채널 반응만 바꾼다 — 이동·환경 충돌 등 다른 채널은 그대로다.
	void SetCombatCollisionEnabled(bool bEnabled);

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

	// 왼쪽만 후보가 2개다(SnS 팩에 오른쪽 1타 모션이 없어서 오른쪽은 1개뿐).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Reaction", meta = (AllowPrivateAccess = "true"))
	TArray<TObjectPtr<UAnimMontage>> HitReactMontages_Front;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Reaction", meta = (AllowPrivateAccess = "true"))
	TArray<TObjectPtr<UAnimMontage>> HitReactMontages_Left;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Reaction", meta = (AllowPrivateAccess = "true"))
	TArray<TObjectPtr<UAnimMontage>> HitReactMontages_Right;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Reaction", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimMontage> DeathMontage_Left;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Reaction", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimMontage> DeathMontage_Right;

#if !UE_BUILD_SHIPPING
private:
	// 체력·자세·구간 태그 등 이 캐릭터 자신이 소유한 정보를 머리 위에 그린다.
	void DrawOwnCombatDebugText() const;
#endif
};
