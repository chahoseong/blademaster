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

	// 어빌리티가 끝나도 몽타주가 끊기지 않게 한다 — 쓰러진 자세를 계속 유지해야 한다.
	// 자동 블렌드 아웃을 끈 몽타주라 "재생 끝남" 델리게이트 자체가 절대 안 온다 — 그래서
	// 완료 콜백을 기다리는 대신, 몽타주 길이만큼 직접 타이머를 걸어 State.Death.Dead로 넘어간다.
	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, DeathMontage, 1.f, NAME_None, /*bStopWhenAbilityEnds=*/ false);
	MontageTask->ReadyForActivation();

	const float DeathMontageLength = DeathMontage->GetPlayLength();
	UE_LOG(LogBlademasterCombat, Verbose, TEXT("%s: 사망 몽타주 %s 재생 시작 (%.2f초 뒤 State.Death.Dead 전환 예정)"),
		*GetNameSafe(Character), *GetNameSafe(DeathMontage), DeathMontageLength);

	UAbilityTask_WaitDelay* DelayTask = UAbilityTask_WaitDelay::WaitDelay(this, DeathMontageLength);
	DelayTask->OnFinish.AddDynamic(this, &UBlademasterGameplayAbility_Death::OnDeathMontageEnded);
	DelayTask->ReadyForActivation();
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

	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}
