#include "Combat/BlademasterGameplayAbility_Death.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "AbilitySystem/BlademasterGameplayEffectContext.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimMontage.h"
#include "BlademasterGameplayTags.h"
#include "BlademasterLogChannels.h"
#include "Characters/BlademasterCharacter.h"

UBlademasterGameplayAbility_Death::UBlademasterGameplayAbility_Death()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	SetAssetTags(FGameplayTagContainer(BlademasterGameplayTags::Ability_Reaction_Death));

	CancelAbilitiesWithTag.AddTag(BlademasterGameplayTags::Ability_Action);
	BlockAbilitiesWithTag.AddTag(BlademasterGameplayTags::Ability_Action);

	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag = BlademasterGameplayTags::GameplayEvent_Reaction_Death;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);
}

void UBlademasterGameplayAbility_Death::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	UAbilitySystemComponent* AbilitySystemComponent = GetAbilitySystemComponentFromActorInfo();
	ABlademasterCharacter* Character = ActorInfo ? Cast<ABlademasterCharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	if (!AbilitySystemComponent || !Character)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AbilitySystemComponent->AddLooseGameplayTag(BlademasterGameplayTags::State_Death_Dying);

	// 무기 채널 충돌을 꺼서 판정 자체에서 빠지게 한다. 부활할 때(GA_Respawn) 다시 켠다.
	Character->SetCombatCollisionEnabled(false);

	const FBlademasterGameplayEffectContext* Context = TriggerEventData ? FBlademasterGameplayEffectContext::FromHandle(TriggerEventData->ContextHandle) : nullptr;
	const EBlademasterHitDirection Direction = Context ? Context->HitDirection : EBlademasterHitDirection::Front;

	UAnimMontage* DeathMontage = Character->GetDeathMontage(Direction);
	if (!DeathMontage)
	{
		UE_LOG(LogBlademasterCombat, Warning, TEXT("%s: 사망 몽타주가 없어 바로 State.Death.Dead로 넘어간다"), *GetNameSafe(Character));
		OnDeathMontageEnded();
		return;
	}

	// 자동 블렌드 아웃을 끈 몽타주라 쓰러진 자세를 유지하고 "재생 끝남" 델리게이트는 오지 않는다 —
	// 그래서 완료 콜백을 기다리는 대신, 몽타주 길이만큼 직접 타이머를 걸어 State.Death.Dead로 넘어간다.
	// 몽타주는 어빌리티가 끝날 때 함께 내려간다(bStopWhenAbilityEnds 기본값). 블렌드 아웃 시간은 몽타주 애셋 값이다.
	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, DeathMontage);
	MontageTask->ReadyForActivation();

	const float DeathMontageLength = DeathMontage->GetPlayLength();
	UE_LOG(LogBlademasterCombat, Verbose, TEXT("%s: 사망 몽타주 %s 재생 시작 (%.2f초 뒤 State.Death.Dead 전환 예정)"),
		*GetNameSafe(Character), *GetNameSafe(DeathMontage), DeathMontageLength);

	UAbilityTask_WaitDelay* DelayTask = UAbilityTask_WaitDelay::WaitDelay(this, DeathMontageLength);
	DelayTask->OnFinish.AddDynamic(this, &UBlademasterGameplayAbility_Death::OnDeathMontageEnded);
	DelayTask->ReadyForActivation();
}

void UBlademasterGameplayAbility_Death::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// 어느 단계에서 끝나든 태그가 남지 않게 둘 다 걷어낸다. 보유하지 않은 태그를 제거해도 보유 수는 음수가 되지 않는다.
	if (UAbilitySystemComponent* AbilitySystemComponent = GetAbilitySystemComponentFromActorInfo())
	{
		AbilitySystemComponent->RemoveLooseGameplayTag(BlademasterGameplayTags::State_Death_Dying);
		AbilitySystemComponent->RemoveLooseGameplayTag(BlademasterGameplayTags::State_Death_Dead);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UBlademasterGameplayAbility_Death::OnDeathMontageEnded()
{
	// State.Death.Dying(사망 몽타주 재생 중) -> State.Death.Dead(부활 대기 중)로 전환한다.
	// Dead를 먼저 붙이고 Dying을 나중에 뗀다 — 부모 State.Death의 보유 수가 0으로 떨어졌다 다시 오르지 않게 한다.
	if (UAbilitySystemComponent* AbilitySystemComponent = GetAbilitySystemComponentFromActorInfo())
	{
		UE_LOG(LogBlademasterCombat, Verbose, TEXT("%s: State.Death.Dying -> State.Death.Dead 전환"), *GetNameSafe(GetAvatarActorFromActorInfo()));
		AbilitySystemComponent->AddLooseGameplayTag(BlademasterGameplayTags::State_Death_Dead);
		AbilitySystemComponent->RemoveLooseGameplayTag(BlademasterGameplayTags::State_Death_Dying);
	}
	else
	{
		UE_LOG(LogBlademasterCombat, Warning, TEXT("OnDeathMontageEnded: AbilitySystemComponent를 못 찾음"));
	}

	// 여기서 EndAbility를 부르지 않는다 — 되살리기가 취소할 때까지 사망 상태 전체 동안 활성으로 남는다.
}
