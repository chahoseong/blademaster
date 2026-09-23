#include "BlademasterGameplayTags.h"

namespace BlademasterGameplayTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Attack, "InputTag.Attack", "공격 입력 액션에 붙는 태그. 어빌리티 스펙의 DynamicSpecSourceTags와 매칭해 입력을 전달한다.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Guard, "InputTag.Guard", "가드 입력 액션에 붙는 태그. 누르는 동안 GA_Guard가 활성이다.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Action,"Ability.Action", "캐릭터가 선택해서 하는 어빌리티(공격 등)의 부모 태그. 피격 등 Reaction류에 끊기고 막힌다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Action_Attack, "Ability.Action.Attack", "공격류 어빌리티(콤보 등)를 식별하는 태그.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Action_Guard, "Ability.Action.Guard", "가드 어빌리티(GA_Guard)를 식별하는 태그. Action류라 피격 반응·붕괴·사망에 끊기고 막힌다.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Reaction_Hit,"Ability.Reaction.Hit", "피격 반응 어빌리티(GA_HitReact)를 식별하는 태그. Action류를 끊고 막는다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Reaction_Death, "Ability.Reaction.Death", "사망 어빌리티(GA_Death)를 식별하는 태그. Action류를 끊고 막는다.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Reaction_Stagger, "Ability.Reaction.Stagger", "붕괴 어빌리티(GA_Stagger)를 식별하는 태그. Action류와 피격 반응을 끊고 막는다. GA_Death가 취소한다.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Reaction_Guard, "Ability.Reaction.Guard", "가드 반응 어빌리티(GA_GuardReact)를 식별하는 태그. 가드는 끊지 않고 공격만 막는다. 붕괴·사망이 취소한다.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Attacking,"State.Attacking", "공격 어빌리티가 활성 상태인 동안 붙어 있다. 락온 추적 등 다른 시스템이 이 태그로 공격 중 여부를 판단한다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Guard, "State.Guard", "가드 입력을 누르고 있어 방어 상태인 동안 붙는다. 라우터가 가드 결과를 고를 때 읽는다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Death,"State.Death", "죽었거나 죽어가는 중. 하위 상태가 공유하는 성질(판정 제외, 락온 후보 제외)을 한 번에 질의하는 부모 태그.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Death_Dying, "State.Death.Dying", "사망 몽타주 재생 중.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Death_Dead, "State.Death.Dead", "사망 몽타주가 끝나고 부활을 기다리는 중. GA_Respawn이 이 태그를 트리거(OwnedTagAdded)로 듣는다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Stagger, "State.Stagger", "자세가 바닥나 붕괴한 중. GA_Stagger가 활성인 동안 붙고, 그동안 자세가 깎이지도 회복되지도 않는다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Posture_RegenDelay,"State.Posture.RegenDelay", "자세가 깎인 뒤 회복이 멈춰 있는 중. 회복 대기 GE가 붙이고, 회복 GE는 이 태그가 있는 동안 실행되지 않는다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attack_Window,"Attack.Window", "한 타 안의 구간 태그들의 부모. 직접 붙이지 않고 하위 구간을 한꺼번에 찾을 때 쓴다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attack_Window_Input, "Attack.Window.Input", "선입력 구간. 이 구간에 눌린 공격 입력을 기억해둔다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attack_Window_Combo, "Attack.Window.Combo", "이어가기 구간. 시작 시 또는 구간 중 공격 입력이 있으면 다음 타로 넘어간다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attack_Window_Cancel, "Attack.Window.Cancel", "캔슬 구간. 이 태그가 있는 동안 다른 어빌리티가 이 공격을 끊을 수 있다(실제로 끊는 로직은 여기 없음).");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayEvent_Weapon_Hit, "GameplayEvent.Weapon.Hit", "칼 판정이 대상을 맞혔을 때 그 대상에게 보내는 이벤트. 맞은 쪽의 CombatComponent가 받는다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayEvent_Reaction_Hit, "GameplayEvent.Reaction.Hit", "CombatComponent가 타격을 처리한 결과가 피격일 때 자신에게 보내는 이벤트. GA_HitReact가 듣는다.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayEvent_Reaction_Death, "GameplayEvent.Reaction.Death", "CombatComponent가 타격을 처리한 결과가 사망일 때 자신에게 보내는 이벤트. GA_Death가 듣는다.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayEvent_Reaction_Stagger, "GameplayEvent.Reaction.Stagger", "CombatComponent가 타격을 처리한 결과가 붕괴 진입일 때 자신에게 보내는 이벤트. GA_Stagger가 듣는다.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayEvent_Reaction_Guard, "GameplayEvent.Reaction.Guard", "CombatComponent가 타격을 처리한 결과가 가드일 때 자신에게 보내는 이벤트. GA_GuardReact가 듣는다.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Damage_Health,"SetByCaller.Damage.Health", "데미지 GE가 체력 메타 어트리뷰트에 넣을 값의 SetByCaller 태그.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Damage_Posture, "SetByCaller.Damage.Posture", "데미지 GE가 자세 메타 어트리뷰트에 넣을 값의 SetByCaller 태그.");
}
