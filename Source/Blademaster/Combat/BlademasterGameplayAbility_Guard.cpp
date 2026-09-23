#include "Combat/BlademasterGameplayAbility_Guard.h"

#include "Abilities/Tasks/AbilityTask_WaitInputRelease.h"
#include "BlademasterGameplayTags.h"
#include "BlademasterLogChannels.h"
#include "Characters/BlademasterCharacter.h"
#include "Combat/BlademasterCombatComponent.h"

namespace
{
	UBlademasterCombatComponent* GetCombatComponent(const FGameplayAbilityActorInfo* ActorInfo)
	{
		const ABlademasterCharacter* Character = ActorInfo ? Cast<ABlademasterCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
		return Character ? Character->GetCombatComponent() : nullptr;
	}
}

UBlademasterGameplayAbility_Guard::UBlademasterGameplayAbility_Guard()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	SetAssetTags(FGameplayTagContainer(BlademasterGameplayTags::Ability_Action_Guard));
	ActivationOwnedTags.AddTag(BlademasterGameplayTags::State_Guard);

	BlockAbilitiesWithTag.AddTag(BlademasterGameplayTags::Ability_Action_Attack);
	ActivationBlockedTags.AddTag(BlademasterGameplayTags::State_Attacking);
}

void UBlademasterGameplayAbility_Guard::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	UE_LOG(LogBlademasterCombat, Log, TEXT("%s: 가드 시작"), *GetNameSafe(GetAvatarActorFromActorInfo()));

	if (UBlademasterCombatComponent* Combat = GetCombatComponent(ActorInfo))
	{
		Combat->SetActiveGuard(this);
	}
	else
	{
		UE_LOG(LogBlademasterCombat, Warning, TEXT("%s: CombatComponent가 없어 가드가 타격을 막지 못한다"), *GetNameSafe(GetAvatarActorFromActorInfo()));
	}

	// 이미 뗐으면 바로 끝난다(bTestAlreadyReleased). 입력 이벤트는 UBlademasterAbilitySystemComponent가 전달한다.
	UAbilityTask_WaitInputRelease* ReleaseTask = UAbilityTask_WaitInputRelease::WaitInputRelease(this, true);
	ReleaseTask->OnRelease.AddDynamic(this, &UBlademasterGameplayAbility_Guard::OnGuardInputReleased);
	ReleaseTask->ReadyForActivation();
}

void UBlademasterGameplayAbility_Guard::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	UE_LOG(LogBlademasterCombat, Log, TEXT("%s: 가드 %s"), *GetNameSafe(ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr),
		bWasCancelled ? TEXT("끊김") : TEXT("해제"));

	// 떼든 끊기든 여기를 지나므로 등록이 남지 않는다.
	if (UBlademasterCombatComponent* Combat = GetCombatComponent(ActorInfo))
	{
		Combat->ClearActiveGuard(this);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

bool UBlademasterGameplayAbility_Guard::CanBlockAttackFrom(const FVector& AttackerLocation) const
{
	const AActor* Avatar = GetAvatarActorFromActorInfo();
	return Avatar && IsWithinFrontArc(Avatar->GetActorLocation(), Avatar->GetActorForwardVector(), AttackerLocation, FrontHalfAngle);
}

bool UBlademasterGameplayAbility_Guard::IsWithinFrontArc(const FVector& VictimLocation, const FVector& VictimForward, const FVector& AttackerLocation, float HalfAngleDegrees)
{
	const FVector ToAttacker = (AttackerLocation - VictimLocation).GetSafeNormal2D();
	if (ToAttacker.IsNearlyZero())
	{
		return true;
	}

	// 경계(정확히 반각)를 부동소수점 오차로 놓치지 않도록 작은 여유를 둔다.
	const float CosHalfAngle = FMath::Cos(FMath::DegreesToRadians(HalfAngleDegrees));
	return FVector::DotProduct(ToAttacker, VictimForward.GetSafeNormal2D()) >= CosHalfAngle - KINDA_SMALL_NUMBER;
}

void UBlademasterGameplayAbility_Guard::OnGuardInputReleased(float TimeHeld)
{
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}
