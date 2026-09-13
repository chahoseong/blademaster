#include "BlademasterGameplayTags.h"

namespace BlademasterGameplayTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Attack, "InputTag.Attack", "공격 입력 액션에 붙는 태그. 어빌리티 스펙의 DynamicSpecSourceTags와 매칭해 입력을 전달한다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Attack, "Ability.Attack", "공격류 어빌리티(콤보 등)를 식별하는 태그.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Attacking, "State.Attacking", "공격 어빌리티가 활성 상태인 동안 붙어 있다. 락온 추적 등 다른 시스템이 이 태그로 공격 중 여부를 판단한다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attack_Window_Input, "Attack.Window.Input", "선입력 구간. 이 구간에 눌린 공격 입력을 기억해둔다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attack_Window_Combo, "Attack.Window.Combo", "이어가기 구간. 시작 시 또는 구간 중 공격 입력이 있으면 다음 타로 넘어간다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attack_Window_Cancel, "Attack.Window.Cancel", "캔슬 구간. 이 태그가 있는 동안 다른 어빌리티가 이 공격을 끊을 수 있다(실제로 끊는 로직은 여기 없음).");
}
