#pragma once

#include "NativeGameplayTags.h"

namespace BlademasterGameplayTags
{
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Attack);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Attack);

	// 공격 어빌리티가 활성 상태인 동안(ActivationOwnedTags) 자동으로 붙는다.
	// 공격류가 아닌 다른 시스템(락온 추적 등)도 "지금 뭔가 하느라 바쁜가"를 물을 때 이 태그를 쓴다.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Attacking);

	// 한 타 안의 구간들. 몽타주의 UBlademasterAnimNotifyState_GameplayTag가 붙였다 뗀다.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attack_Window_Input);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attack_Window_Combo);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attack_Window_Cancel);

	// 칼 판정이 대상을 맞혔을 때 그 대상에게 SendGameplayEventToActor로 보내는 이벤트 태그.
	// 맞은 쪽의 피격 어빌리티가 AbilityTriggers로 이 태그를 듣고 활성화된다.
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayEvent_WeaponHit);
}
