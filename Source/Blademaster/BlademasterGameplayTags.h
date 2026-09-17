#pragma once

#include "NativeGameplayTags.h"

namespace BlademasterGameplayTags
{
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Attack);

	// 캐릭터가 "선택해서" 하는 어빌리티(공격 등). 피격 등 Reaction류에 끊기고, 그동안 활성화가 막힌다.
	// 새 Action 어빌리티는 이 부모 태그의 자식 태그(예: Ability.Action.Dodge)만 붙이면
	// 별도 코드 수정 없이 자동으로 적용받는다.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Action);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Action_Attack);

	// 캐릭터 의지와 무관하게 강제로 일어나는 어빌리티(피격 등). Action류를 끊고 막는다.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Reaction_Hit);

	// 공격 어빌리티가 활성 상태인 동안(ActivationOwnedTags) 자동으로 붙는다.
	// 공격류가 아닌 다른 시스템(락온 추적 등)도 "지금 뭔가 하느라 바쁜가"를 물을 때 이 태그를 쓴다.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Attacking);

	// 사망 몽타주 재생 중. 판정(충돌로 처리)·락온 후보에서 제외하는 기준이 된다.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Dying);

	// 사망 몽타주가 끝나고 부활을 기다리는 중. State_Dying과 마찬가지로 락온 후보에서 제외된다.
	// GA_Respawn이 이 태그를 트리거(OwnedTagAdded)로 듣는다.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Dead);

	// 한 타 안의 구간들. 몽타주의 UBlademasterAnimNotifyState_GameplayTag가 붙였다 뗀다.
	// 부모 태그(Attack_Window)는 직접 붙이지 않고, 하위 구간을 한꺼번에 찾을 때만 쓴다.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attack_Window);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attack_Window_Input);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attack_Window_Combo);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attack_Window_Cancel);

	// 칼 판정이 대상을 맞혔을 때 그 대상에게 SendGameplayEventToActor로 보내는 이벤트 태그.
	// 맞은 쪽의 피격 어빌리티가 AbilityTriggers로 이 태그를 듣고 활성화된다.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayEvent_WeaponHit);

	// 데미지 GE의 SetByCaller 매그니튜드 태그. 피격 어빌리티가 컨텍스트의 값을 여기 실어 스펙에 넣는다.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Damage_Health);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Damage_Posture);
}
