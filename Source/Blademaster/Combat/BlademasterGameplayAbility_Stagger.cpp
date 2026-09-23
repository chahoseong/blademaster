#include "Combat/BlademasterGameplayAbility_Stagger.h"

#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "AbilitySystem/BlademasterAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "BlademasterGameplayTags.h"
#include "BlademasterLogChannels.h"
#include "Characters/BlademasterCharacter.h"
#include "GameplayEffect.h"

UBlademasterGameplayAbility_Stagger::UBlademasterGameplayAbility_Stagger()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	SetAssetTags(FGameplayTagContainer(BlademasterGameplayTags::Ability_Reaction_Stagger));
	ActivationOwnedTags.AddTag(BlademasterGameplayTags::State_Stagger);

	// 스스로 하는 행동(Ability.Action 계열)과 피격 반응을 끊고, 붕괴가 끝날 때까지 막는다.
	CancelAbilitiesWithTag.AddTag(BlademasterGameplayTags::Ability_Action);
	CancelAbilitiesWithTag.AddTag(BlademasterGameplayTags::Ability_Reaction_Hit);
	CancelAbilitiesWithTag.AddTag(BlademasterGameplayTags::Ability_Reaction_Guard);
	BlockAbilitiesWithTag.AddTag(BlademasterGameplayTags::Ability_Action);
	BlockAbilitiesWithTag.AddTag(BlademasterGameplayTags::Ability_Reaction_Hit);
	ActivationBlockedTags.AddTag(BlademasterGameplayTags::State_Death);

	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag = BlademasterGameplayTags::GameplayEvent_Reaction_Stagger;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);
}

void UBlademasterGameplayAbility_Stagger::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	UAbilitySystemComponent* AbilitySystemComponent = GetAbilitySystemComponentFromActorInfo();
	ABlademasterCharacter* Character = ActorInfo ? Cast<ABlademasterCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	const UBlademasterAttributeSet* AttributeSet = AbilitySystemComponent ? AbilitySystemComponent->GetSet<UBlademasterAttributeSet>() : nullptr;
	if (!Character || !AttributeSet)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	Character->SetMovementEnabled(false);

	// 지속 시간은 진입 시점에 정해진다. 이후의 타격은 이 대기를 건드리지 않는다(R-5).
	const float Duration = AttributeSet->GetStaggerDuration();
	UE_LOG(LogBlademasterCombat, Log, TEXT("%s: 붕괴 시작 (%.2f초)"), *GetNameSafe(Character), Duration);

	UAbilityTask_WaitDelay* DelayTask = UAbilityTask_WaitDelay::WaitDelay(this, Duration);
	DelayTask->OnFinish.AddDynamic(this, &UBlademasterGameplayAbility_Stagger::OnStaggerDurationElapsed);
	DelayTask->ReadyForActivation();
}

void UBlademasterGameplayAbility_Stagger::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// 어떻게 끝나든(시간 경과, 사망에 의한 취소) 이동 금지를 걷는다. State.Stagger는 ActivationOwnedTags라 함께 떨어진다.
	if (ABlademasterCharacter* Character = ActorInfo ? Cast<ABlademasterCharacter>(ActorInfo->AvatarActor.Get()) : nullptr)
	{
		Character->SetMovementEnabled(true);

		if (bWasCancelled)
		{
			UE_LOG(LogBlademasterCombat, Log, TEXT("%s: 붕괴가 취소됐다 — 자세를 되돌리지 않는다"), *GetNameSafe(Character));
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UBlademasterGameplayAbility_Stagger::OnStaggerDurationElapsed()
{
	UAbilitySystemComponent* AbilitySystemComponent = GetAbilitySystemComponentFromActorInfo();
	AActor* Avatar = GetAvatarActorFromActorInfo();

	// 끝나는 순간 자세를 최대치로 되돌린다(R-6). 바로 아래 EndAbility가 금지를 푸는 것과 같은 프레임이다.
	if (AbilitySystemComponent && PostureRestoreEffectClass)
	{
		FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
		EffectContext.AddSourceObject(Avatar);
		const FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(PostureRestoreEffectClass, 1.f, EffectContext);
		if (SpecHandle.IsValid())
		{
			AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
		}
	}
	else if (!PostureRestoreEffectClass)
	{
		UE_LOG(LogBlademasterCombat, Warning, TEXT("%s: PostureRestoreEffectClass가 지정되지 않아 붕괴가 끝나도 자세가 돌아오지 않는다"), *GetNameSafe(Avatar));
	}

	const UBlademasterAttributeSet* AttributeSet = AbilitySystemComponent ? AbilitySystemComponent->GetSet<UBlademasterAttributeSet>() : nullptr;
	UE_LOG(LogBlademasterCombat, Log, TEXT("%s: 붕괴 종료 — 자세 %.1f/%.1f"), *GetNameSafe(Avatar),
		AttributeSet ? AttributeSet->GetPosture() : 0.f, AttributeSet ? AttributeSet->GetMaxPosture() : 0.f);

	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}
