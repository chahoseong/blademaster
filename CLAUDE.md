# Blademaster

UE 5.8. 자세(Posture)를 겨루는 검·방패 근접 전투.

- 목표·범위·완료 기준·진행 계획: `Docs/ROADMAP.md`
- 규칙 스펙: `Specs/`
- 작업 단위와 결정 기록: GitHub 이슈 (`chahoseong/blademaster`)

## 작업 방식

- 구현에 앞서 설계 대안과 근거를 제시하고, 선택은 사용자가 한다. 결정과 이유는 이슈에 남긴다.
- `Docs/ROADMAP.md`의 단계 항목은 확정된 규칙이 아니라 계획이다. 그 단계에 착수할 때 논의로 확정한다.
- `Docs/Notes/`는 개인 학습용이다. 이슈·커밋·프로젝트 문서에서 참조하지 않는다.

## 이름 규칙

접두사 `Blademaster`를 붙이는 것 — C++ 클래스(`ABlademasterCharacter`), 충돌 프로필
(`BlademasterCharacterMesh`), 콘솔 변수(`Blademaster.Debug.*`), 로그 카테고리
(`LogBlademasterCombat`). 엔진·플러그인이 만든 이름과 같은 목록에 섞이기 때문이다.

접두사를 붙이지 않는 것 — 트레이스 채널(`Weapon`), 게임플레이 태그(`Ability.Attack`,
`InputTag.Attack`). 태그는 계층으로 나눈다.

네이티브 게임플레이 태그는 `BlademasterGameplayTags` 네임스페이스에 선언하고, 변수 이름에
`TAG_` 접두사를 붙이지 않는다 (`BlademasterGameplayTags::InputTag_Attack`,
`Source/Blademaster/BlademasterGameplayTags.h/.cpp`).
