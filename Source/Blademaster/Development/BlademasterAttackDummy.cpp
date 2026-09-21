#include "Development/BlademasterAttackDummy.h"

#include "AbilitySystem/BlademasterAttributeSet.h"
#include "AbilitySystem/BlademasterGameplayEffectContext.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "BlademasterCollisionChannels.h"
#include "BlademasterGameplayTags.h"
#include "BlademasterLogChannels.h"
#include "Characters/BlademasterCharacter.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/World.h"
#include "GameplayEffect.h"

#if !UE_BUILD_SHIPPING
#include "BlademasterDebug.h"
#endif

namespace
{
	// 공격자 위치에서 대상을 바라봤을 때, 대상 위치를 가운데에 두고 수평으로 가로지르는 선분을 구한다.
	// Direction은 공격자 시점의 좌우다. 공격자와 대상이 수평으로 겹쳐 방향을 정할 수 없으면 false.
	bool ComputeSweepSegment(const FVector& AttackerLocation, const FVector& TargetLocation,
		EBlademasterAttackDummySweepDirection Direction, float Length, FVector& OutStart, FVector& OutEnd)
	{
		const FVector Forward = FVector(TargetLocation.X - AttackerLocation.X, TargetLocation.Y - AttackerLocation.Y, 0.f).GetSafeNormal();
		if (Forward.IsNearlyZero())
		{
			return false;
		}

		// 공격자가 대상을 바라볼 때의 오른쪽. X 앞, Y 오른쪽, Z 위.
		const FVector Right = FVector::CrossProduct(FVector::UpVector, Forward);
		const FVector SwingDirection = Direction == EBlademasterAttackDummySweepDirection::LeftToRight ? Right : -Right;
		const FVector HalfSwing = SwingDirection * (Length * 0.5f);

		OutStart = TargetLocation - HalfSwing;
		OutEnd = TargetLocation + HalfSwing;
		return true;
	}
}

ABlademasterAttackDummy::ABlademasterAttackDummy()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	TargetTrigger = CreateDefaultSubobject<USphereComponent>(TEXT("TargetTrigger"));
	TargetTrigger->SetupAttachment(RootComponent);
	TargetTrigger->SetCollisionProfileName(TEXT("Trigger"));
	TargetTrigger->InitSphereRadius(500.f);

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));

	AttributeSet = CreateDefaultSubobject<UBlademasterAttributeSet>(TEXT("AttributeSet"));
}

void ABlademasterAttackDummy::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const float Now = GetWorld()->GetTimeSeconds();
	ABlademasterCharacter* Target = GetCurrentTarget();

	if (bTelegraphing)
	{
		if (Target != TelegraphTarget.Get())
		{
			// 예고하던 대상이 트리거를 벗어났다 — 이 주기는 타격하지 않는다.
			UE_LOG(LogBlademasterCombat, Log, TEXT("%s: 예고 취소 — 대상 %s가 트리거를 벗어났다"), *GetName(), *GetNameSafe(TelegraphTarget.Get()));
			bTelegraphing = false;
			TelegraphTarget.Reset();
		}
		else if (Now - TelegraphStartTime >= TelegraphDuration)
		{
			Strike(Target, Now - TelegraphStartTime);
			bTelegraphing = false;
			TelegraphTarget.Reset();
			NextTelegraphTime = Now + AttackInterval;
		}
	}

	if (!bTelegraphing && Target && Now >= NextTelegraphTime)
	{
		StartTelegraph(Target, Now);
	}

#if !UE_BUILD_SHIPPING
	DrawAttributeText();
#endif
}

UAbilitySystemComponent* ABlademasterAttackDummy::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ABlademasterAttackDummy::BeginPlay()
{
	Super::BeginPlay();

	TargetTrigger->OnComponentBeginOverlap.AddDynamic(this, &ABlademasterAttackDummy::OnTargetTriggerBeginOverlap);
	TargetTrigger->OnComponentEndOverlap.AddDynamic(this, &ABlademasterAttackDummy::OnTargetTriggerEndOverlap);

	// 바인딩 전에 이미 트리거 안에 있던 캐릭터도 후보로 받는다.
	TArray<AActor*> OverlappingActors;
	TargetTrigger->GetOverlappingActors(OverlappingActors, ABlademasterCharacter::StaticClass());
	for (AActor* Actor : OverlappingActors)
	{
		AddTargetCandidate(Actor);
	}

	if (!InitializeAttributesEffect)
	{
		UE_LOG(LogBlademasterCombat, Warning, TEXT("%s: InitializeAttributesEffect가 지정되지 않아 체력·자세가 0으로 남는다"), *GetName());
		return;
	}

	FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
	EffectContext.AddSourceObject(this);

	const FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(InitializeAttributesEffect, 1.f, EffectContext);
	if (SpecHandle.IsValid())
	{
		AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
}

void ABlademasterAttackDummy::OnTargetTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AddTargetCandidate(OtherActor);
}

void ABlademasterAttackDummy::OnTargetTriggerEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex)
{
	// 한 액터의 여러 컴포넌트가 겹쳐 있을 수 있다. 액터가 완전히 벗어났을 때만 뺀다.
	if (OtherActor && !TargetTrigger->IsOverlappingActor(OtherActor))
	{
		TargetCandidates.Remove(Cast<ABlademasterCharacter>(OtherActor));
	}
}

void ABlademasterAttackDummy::AddTargetCandidate(AActor* Actor)
{
	if (ABlademasterCharacter* Character = Cast<ABlademasterCharacter>(Actor))
	{
		TargetCandidates.AddUnique(Character);
	}
}

ABlademasterCharacter* ABlademasterAttackDummy::GetCurrentTarget()
{
	TargetCandidates.RemoveAll([](const TWeakObjectPtr<ABlademasterCharacter>& Candidate) { return !Candidate.IsValid(); });
	return TargetCandidates.Num() > 0 ? TargetCandidates[0].Get() : nullptr;
}

void ABlademasterAttackDummy::StartTelegraph(ABlademasterCharacter* Target, float Now)
{
	bTelegraphing = true;
	TelegraphStartTime = Now;
	TelegraphTarget = Target;

	UE_LOG(LogBlademasterCombat, Log, TEXT("%s: 예고 시작 — 대상 %s, 거리 %.0f, 예고 시간 %.2f초"),
		*GetName(), *GetNameSafe(Target), FVector::Dist2D(GetActorLocation(), Target->GetActorLocation()), TelegraphDuration);
}

void ABlademasterAttackDummy::Strike(ABlademasterCharacter* Target, float ElapsedSinceTelegraph)
{
	const FVector TargetLocation = Target->GetActorLocation();

	FVector SweepStart;
	FVector SweepEnd;
	if (!ComputeSweepSegment(GetActorLocation(), TargetLocation, SweepDirection, SweepLength, SweepStart, SweepEnd))
	{
		UE_LOG(LogBlademasterCombat, Warning, TEXT("%s: 대상 %s와 수평으로 겹쳐 스윕 방향을 정할 수 없다"), *GetName(), *GetNameSafe(Target));
		return;
	}

	// 칼을 휘두르는 쪽처럼 대상을 향해 돈다. 스윕 선분은 위치만으로 정해지므로 판정에는 영향이 없다.
	SetActorRotation(FRotator(0.f, (TargetLocation - GetActorLocation()).Rotation().Yaw, 0.f));

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	TArray<FHitResult> HitResults;
	GetWorld()->SweepMultiByChannel(HitResults, SweepStart, SweepEnd, FQuat::Identity,
		BlademasterCollisionChannels::Weapon, FCollisionShape::MakeSphere(SweepRadius), QueryParams);

	const FHitResult* TargetHit = HitResults.FindByPredicate([Target](const FHitResult& Hit) { return Hit.GetActor() == Target; });
	if (!TargetHit)
	{
		UE_LOG(LogBlademasterCombat, Log, TEXT("%s: 타격 — 예고 후 %.3f초, 대상 %s를 맞히지 못했다"), *GetName(), ElapsedSinceTelegraph, *GetNameSafe(Target));
		return;
	}

	FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
	EffectContext.AddHitResult(*TargetHit);

	if (FBlademasterGameplayEffectContext* BlademasterContext = FBlademasterGameplayEffectContext::FromHandle(EffectContext))
	{
		BlademasterContext->HealthDamage = HealthDamage;
		BlademasterContext->PostureDamage = PostureDamage;
	}

	FGameplayEventData Payload;
	Payload.EventTag = BlademasterGameplayTags::GameplayEvent_Weapon_Hit;
	Payload.Instigator = this;
	Payload.Target = Target;
	Payload.ContextHandle = EffectContext;

	UE_LOG(LogBlademasterCombat, Log, TEXT("%s: 타격 — 예고 후 %.3f초, 대상 %s의 %s 부위, 체력 피해 %.1f, 자세 피해 %.1f"),
		*GetName(), ElapsedSinceTelegraph, *GetNameSafe(Target), *TargetHit->BoneName.ToString(), HealthDamage, PostureDamage);

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Target, Payload.EventTag, Payload);
}

#if !UE_BUILD_SHIPPING
void ABlademasterAttackDummy::DrawAttributeText() const
{
	const UBlademasterAttributeSet* RegisteredSet = AbilitySystemComponent ? AbilitySystemComponent->GetSet<UBlademasterAttributeSet>() : nullptr;
	if (!RegisteredSet)
	{
		BlademasterDebug::DrawDebugTextLine(this, 0, TEXT("AttributeSet 미등록"), FColor::Red);
		return;
	}

	BlademasterDebug::DrawDebugTextLine(this, 0, FString::Printf(TEXT("HP: %.0f / %.0f"), RegisteredSet->GetHealth(), RegisteredSet->GetMaxHealth()));
	BlademasterDebug::DrawDebugTextLine(this, 1, FString::Printf(TEXT("Posture: %.0f / %.0f"), RegisteredSet->GetPosture(), RegisteredSet->GetMaxPosture()));
}
#endif
