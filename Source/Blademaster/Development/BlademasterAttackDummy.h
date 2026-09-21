#pragma once

#include "AbilitySystemInterface.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BlademasterAttackDummy.generated.h"

class ABlademasterCharacter;
class UAbilitySystemComponent;
class UBlademasterAttributeSet;
class UGameplayEffect;
class UPrimitiveComponent;
class USphereComponent;
struct FHitResult;

// 장치 자신이 대상을 바라본 시점에서 칼을 휘두르는 방향.
UENUM()
enum class EBlademasterAttackDummySweepDirection : uint8
{
	LeftToRight,
	RightToLeft
};

// 가드·패링 검증용 공격 장치. 레벨에 배치해 쓰며 전투 캐릭터의 구성에 포함하지 않는다.
//
// 라우터(UBlademasterCombatComponent)가 공격자에게 요구하는 것만 갖는다 — IAbilitySystemInterface로
// ASC를 돌려주고(데미지 GE 스펙을 이 ASC로 만든다), 패링 시 깎일 UBlademasterAttributeSet을 갖는다.
// ASC는 InitializeComponent에서 이 액터를 Owner·Avatar로 초기화하고, 이 액터의 AttributeSet
// 서브오브젝트를 스스로 등록한다.
//
// 트리거 안에 들어온 캐릭터를 대상으로 예고 → 타격 → 휴지를 반복한다. 타격은 몽타주 대신
// 대상의 현재 위치를 가로지르는 Weapon 채널 스윕 한 번이고, 맞으면 GameplayEvent.Weapon.Hit을
// 보낸다. 타격 시점은 예고 시작에서 TelegraphDuration만큼 뒤로 고정되어 거리와 무관하다.
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

	// 이 안에 들어온 ABlademasterCharacter가 대상이 된다. 반경은 배치된 인스턴스에서 편집한다.
	UPROPERTY(VisibleAnywhere, Category = "AttackDummy")
	TObjectPtr<USphereComponent> TargetTrigger;

	// BeginPlay에 한 번 적용해 체력·자세를 초기화한다. 캐릭터가 쓰는 초기화 GE 에셋을 그대로 지정한다.
	UPROPERTY(EditAnywhere, Category = "AttackDummy")
	TSubclassOf<UGameplayEffect> InitializeAttributesEffect;

	// 아래 값들은 매 틱 읽으므로 실험 중에 바꾸면 바로 적용된다.

	// 타격이 끝난 뒤 다음 예고를 시작할 때까지 쉬는 시간(초).
	UPROPERTY(EditAnywhere, Category = "AttackDummy", meta = (ClampMin = "0", Units = "s"))
	float AttackInterval = 2.f;

	// 예고 시작부터 타격까지의 시간(초).
	UPROPERTY(EditAnywhere, Category = "AttackDummy", meta = (ClampMin = "0", Units = "s"))
	float TelegraphDuration = 1.f;

	UPROPERTY(EditAnywhere, Category = "AttackDummy")
	EBlademasterAttackDummySweepDirection SweepDirection = EBlademasterAttackDummySweepDirection::LeftToRight;

	// 스윕 선분의 전체 길이. 대상 위치가 가운데다.
	UPROPERTY(EditAnywhere, Category = "AttackDummy", meta = (ClampMin = "0", Units = "cm"))
	float SweepLength = 200.f;

	UPROPERTY(EditAnywhere, Category = "AttackDummy", meta = (ClampMin = "0", Units = "cm"))
	float SweepRadius = 10.f;

	UPROPERTY(EditAnywhere, Category = "AttackDummy", meta = (ClampMin = "0"))
	float HealthDamage = 10.f;

	UPROPERTY(EditAnywhere, Category = "AttackDummy", meta = (ClampMin = "0"))
	float PostureDamage = 20.f;

private:
	UFUNCTION()
	void OnTargetTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnTargetTriggerEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex);

	void AddTargetCandidate(AActor* Actor);

	// 트리거에 먼저 들어온 순서로 첫 번째 유효한 후보.
	ABlademasterCharacter* GetCurrentTarget();

	void StartTelegraph(ABlademasterCharacter* Target, float Now);
	void Strike(ABlademasterCharacter* Target, float ElapsedSinceTelegraph);

	TArray<TWeakObjectPtr<ABlademasterCharacter>> TargetCandidates;

	bool bTelegraphing = false;
	float TelegraphStartTime = 0.f;
	float NextTelegraphTime = 0.f;
	TWeakObjectPtr<ABlademasterCharacter> TelegraphTarget;

#if !UE_BUILD_SHIPPING
	// ASC에 등록된 AttributeSet에서 체력·자세를 읽어 장치 위에 그린다.
	void DrawAttributeText() const;
#endif
};
