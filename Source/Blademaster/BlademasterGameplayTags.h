#pragma once

#include "NativeGameplayTags.h"

namespace BlademasterGameplayTags
{
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Attack);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Guard);

	// 캐릭터가 "선택해서" 하는 어빌리티(공격 등). 피격 등 Reaction류에 끊기고, 그동안 활성화가 막힌다.
	// 새 Action 어빌리티는 이 부모 태그의 자식 태그(예: Ability.Action.Dodge)만 붙이면
	// 별도 코드 수정 없이 자동으로 적용받는다.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Action);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Action_Attack);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Action_Guard);

	// 캐릭터 의지와 무관하게 강제로 일어나는 어빌리티(피격 등). Action류를 끊고 막는다.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Reaction_Hit);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Reaction_Death);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Reaction_Stagger);

	// 공격 어빌리티가 활성 상태인 동안(ActivationOwnedTags) 자동으로 붙는다.
	// 공격류가 아닌 다른 시스템(락온 추적 등)도 "지금 뭔가 하느라 바쁜가"를 물을 때 이 태그를 쓴다.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Attacking);

	// 가드 입력을 누르고 있어 방어 상태인 동안(GA_Guard의 ActivationOwnedTags) 붙는다. 라우터가 가드 결과를 고를 때 읽는다.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Guard);

	// 죽었거나 죽어가는 중. 두 하위 상태가 공유하는 성질(무기 판정 제외, 락온 후보 제외)을
	// 한 번에 질의할 때 쓰는 부모 태그다. 직접 붙이지 않고 하위 태그를 통해 보유된다.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Death);

	// 사망 몽타주 재생 중.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Death_Dying);

	// 사망 몽타주가 끝나고 부활을 기다리는 중. GA_Respawn이 이 태그를 트리거(OwnedTagAdded)로 듣는다.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Death_Dead);

	// 자세가 바닥나 붕괴한 중(Specs/002-stagger.md). GA_Stagger가 활성인 동안 붙는다.
	// 데미지 GE의 자세 모디파이어와 회복 GE가 이 태그 동안 자세를 바꾸지 않는다.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Stagger);

	// 자세가 깎인 뒤 회복이 멈춰 있는 중. 회복 대기 GE가 지속 시간 동안 붙이고, 회복 GE는 이 태그가 있는 동안 실행되지 않는다.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Posture_RegenDelay);

	// 한 타 안의 구간들. 몽타주의 UBlademasterAnimNotifyState_GameplayTag가 붙였다 뗀다.
	// 부모 태그(Attack_Window)는 직접 붙이지 않고, 하위 구간을 한꺼번에 찾을 때만 쓴다.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attack_Window);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attack_Window_Input);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attack_Window_Combo);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attack_Window_Cancel);

	// 칼 판정이 대상을 맞혔을 때 그 대상에게 SendGameplayEventToActor로 보내는 이벤트 태그(원인).
	// 맞은 쪽의 CombatComponent가 받아 처리한다.
	// 부모 태그(GameplayEvent.Weapon)에는 트리거를 걸지 않는다 — 모든 하위 이벤트에 함께 활성화된다.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayEvent_Weapon_Hit);

	// CombatComponent가 타격을 처리한 뒤 자신에게 보내는 결과 이벤트. 반응 어빌리티가 AbilityTriggers로 듣는다.
	// 부모 태그(GameplayEvent.Reaction)에는 트리거를 걸지 않는다.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayEvent_Reaction_Hit);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayEvent_Reaction_Death);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayEvent_Reaction_Stagger);

	// 데미지 GE의 SetByCaller 매그니튜드 태그. CombatComponent가 컨텍스트의 값을 여기 실어 스펙에 넣는다.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Damage_Health);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Damage_Posture);
}
