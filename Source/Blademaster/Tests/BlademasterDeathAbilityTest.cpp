#include "AbilitySystemComponent.h"
#include "BlademasterCollisionChannels.h"
#include "BlademasterGameplayTags.h"
#include "Characters/BlademasterCharacter.h"
#include "Combat/BlademasterGameplayAbility_Death.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameplayAbilitySpec.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// GA_Death의 수명 계약: 어빌리티의 수명이 곧 사망 상태의 수명이다.
// Dead로 넘어간 뒤에도 활성 상태로 남아 Ability.Action을 계속 막고, 밖에서 취소하면
// 어느 단계였든 State.Death 하위 태그가 모두 걷힌다.
// 몽타주가 없는 네이티브 캐릭터를 쓴다 — 그러면 Dying을 거치는 즉시 Dead로 넘어가는 경로를 탄다.
// 지연 후 전환, 몽타주 재생·복귀, GA_Respawn의 대기와 어트리뷰트 복원은 PIE에서 확인한다.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBlademasterDeathAbilityLifetimeTest, "Blademaster.Combat.DeathAbilityLifetime",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBlademasterDeathAbilityLifetimeTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("테스트 월드 생성"), World))
	{
		return false;
	}

	FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
	WorldContext.SetCurrentWorld(World);
	World->InitializeActorsForPlay(FURL());
	World->BeginPlay();

	ABlademasterCharacter* Character = World->SpawnActor<ABlademasterCharacter>();
	UAbilitySystemComponent* AbilitySystemComponent = Character ? Character->GetAbilitySystemComponent() : nullptr;

	bool bSetUp = TestNotNull(TEXT("캐릭터 생성"), Character) && TestNotNull(TEXT("캐릭터의 ASC"), AbilitySystemComponent);
	if (bSetUp)
	{
		AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(UBlademasterGameplayAbility_Death::StaticClass(), 1, INDEX_NONE, Character));

		const FGameplayTagContainer ActionTags(BlademasterGameplayTags::Ability_Action_Attack);
		const FGameplayTagContainer DeathAbilityTags(BlademasterGameplayTags::Ability_Reaction_Death);

		auto SendDeathEvent = [&]()
		{
			FGameplayEventData Payload;
			Payload.EventTag = BlademasterGameplayTags::GameplayEvent_Reaction_Death;
			Payload.ContextHandle = AbilitySystemComponent->MakeEffectContext();
			return AbilitySystemComponent->HandleGameplayEvent(Payload.EventTag, &Payload);
		};

		auto IsDeathAbilityActive = [&]()
		{
			const FGameplayAbilitySpec* Spec = AbilitySystemComponent->FindAbilitySpecFromClass(UBlademasterGameplayAbility_Death::StaticClass());
			return Spec && Spec->IsActive();
		};

		// 사망: Dead 단계까지 가고, 어빌리티는 끝나지 않으며, 공격이 막히고, 무기 판정에서 빠진다.
		TestEqual(TEXT("사망 이벤트로 어빌리티가 활성화된다"), SendDeathEvent(), 1);
		TestTrue(TEXT("Dead 단계에 들어선다"), AbilitySystemComponent->HasMatchingGameplayTag(BlademasterGameplayTags::State_Death_Dead));
		TestFalse(TEXT("Dead에 들어서면 Dying은 떨어진다"), AbilitySystemComponent->HasMatchingGameplayTag(BlademasterGameplayTags::State_Death_Dying));
		TestTrue(TEXT("Dead 단계에서도 어빌리티가 활성 상태로 남는다"), IsDeathAbilityActive());
		TestTrue(TEXT("사망 중에는 공격류 어빌리티가 막힌다"), AbilitySystemComponent->AreAbilityTagsBlocked(ActionTags));
		TestEqual(TEXT("무기 채널 충돌이 꺼진다"), Character->GetMesh()->GetCollisionResponseToChannel(BlademasterCollisionChannels::Weapon), ECR_Ignore);

		// 취소: 태그와 차단이 모두 걷힌다.
		AbilitySystemComponent->CancelAbilities(&DeathAbilityTags);
		TestFalse(TEXT("취소하면 어빌리티가 끝난다"), IsDeathAbilityActive());
		TestFalse(TEXT("취소하면 State.Death 하위 태그가 모두 없어진다"), AbilitySystemComponent->HasMatchingGameplayTag(BlademasterGameplayTags::State_Death));
		TestFalse(TEXT("취소하면 공격류 차단이 풀린다"), AbilitySystemComponent->AreAbilityTagsBlocked(ActionTags));

		// 취소 후 다시 사망해도 같은 순서로 반복된다.
		TestEqual(TEXT("다시 사망 이벤트를 받으면 다시 활성화된다"), SendDeathEvent(), 1);
		TestTrue(TEXT("두 번째 사망도 Dead 단계에 들어선다"), AbilitySystemComponent->HasMatchingGameplayTag(BlademasterGameplayTags::State_Death_Dead));

		// Dying 단계에서 끝나는 경우를 흉내 낸다: Dying을 직접 붙여 둔 채 취소해도 둘 다 걷힌다.
		AbilitySystemComponent->AddLooseGameplayTag(BlademasterGameplayTags::State_Death_Dying);
		AbilitySystemComponent->CancelAbilities(&DeathAbilityTags);
		TestFalse(TEXT("Dying이 붙은 채 취소해도 State.Death 하위 태그가 모두 없어진다"), AbilitySystemComponent->HasMatchingGameplayTag(BlademasterGameplayTags::State_Death));
	}

	if (Character)
	{
		Character->Destroy();
	}
	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
