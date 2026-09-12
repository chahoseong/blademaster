# 003 — 권한(Authority) 모델과 리플리케이션

- 기준 엔진: UE 5.8

---

## 1. 한 문장 요약

언리얼의 멀티플레이는 **"서버가 진실, 클라이언트는 그 사본"** 모델이다.
상태를 바꿀 권리는 서버에만 있고, 클라이언트는 결과를 받아서 보여주거나 서버에게 부탁한다.

싱글플레이도 예외가 아니다. Standalone으로 실행하면 그 프로세스가 곧 서버라
`HasAuthority()`가 항상 true다. 그래서 **권한 체크를 지금 넣어 두면 싱글에서는 비용이 없고,
멀티를 켜는 순간 그대로 동작한다.**

## 2. 실행 구성

| 구성 | 설명 |
|---|---|
| Standalone | 혼자 실행. 모든 액터가 권한을 가진다 |
| Listen Server | 한 사람이 서버이면서 플레이어 |
| Dedicated Server | 화면 없는 순수 서버. 렌더링·입력·UI 코드가 실행되면 안 되는 환경 |
| Client | 서버에 접속만 한다 |

## 3. Role — 같은 액터도 기계마다 역할이 다르다

`ENetRole`은 네 가지다 (`EngineTypes.h:3582`).
`ROLE_None`, `ROLE_SimulatedProxy`, `ROLE_AutonomousProxy`, `ROLE_Authority`

핵심은 **같은 액터라도 어느 기계에서 보느냐에 따라 Role이 다르다**는 것이다.
플레이어 A의 캐릭터를 예로 들면:

| 보는 곳 | LocalRole | 의미 |
|---|---|---|
| 서버 | `Authority` | 진짜 상태를 가진 원본 |
| A의 클라이언트 | `AutonomousProxy` | 내가 조종하는 사본. 입력과 예측이 여기서 일어난다 |
| B의 클라이언트 | `SimulatedProxy` | 남의 캐릭터. 받은 값으로 보간해 보여주기만 한다 |

- `HasAuthority()`는 `GetLocalRole() == ROLE_Authority`다.
- `AutonomousProxy`가 되는 지점: `APawn::PossessedBy`에서 플레이어 컨트롤러가 원격이면
  `SetAutonomousProxy(true)`를 호출한다 (`Pawn.cpp:688-698`).
- Autonomous와 Simulated를 나누는 이유: **내 캐릭터만 입력에 즉시 반응해야 하고(예측),
  남의 캐릭터는 늦게 도착한 값으로 부드럽게 보여주면 되기 때문**이다.

## 4. 동기화 수단: 프로퍼티 리플리케이션

- 액터가 `bReplicates = true`이고 프로퍼티를 `GetLifetimeReplicatedProps`에 등록하면
  **서버 → 클라이언트 단방향**으로 값이 전파된다. 반대 방향은 없다.
- `COND_*` 조건으로 "누구에게 보낼지"를 좁힌다 (`COND_OwnerOnly`, `COND_SkipOwner`, `COND_Never` 등).
- **핵심 성질: 프로퍼티는 "최종 값"만 보장하고 중간 변화는 유실될 수 있다.**
  체력이 100 → 90 → 80으로 바뀌어도 클라이언트는 80만 볼 수 있다.
  - "값이 바뀌었다"가 아니라 "맞았다"는 **사건**을 전달하려면 프로퍼티로는 부족하다.
    RPC나 GameplayCue를 쓴다.
  - 값이 이전과 같아도 알림을 받아야 하면 `REPNOTIFY_Always`를 쓴다 (→ [AttributeSet에 이 옵션을 쓴 이유](#8-현재-코드에-적용된-것)).

## 5. 동기화 수단: RPC

| 종류 | 방향 | 용도 |
|---|---|---|
| `Server` | 클라이언트 → 서버 | "이 행동을 하고 싶다"는 요청. 신뢰할 수 없는 입력이므로 서버가 검증한다 |
| `Client` | 서버 → 소유 클라이언트 | 그 플레이어에게만 필요한 통지 |
| `NetMulticast` | 서버 → 모든 클라이언트 | 모두가 봐야 하는 연출 |

- `Reliable`은 도착을 보장하지만 대역폭을 쓰고 순서를 막는다. 매 프레임 나가는 것에 쓰면 안 된다.
- **RPC가 전달되려면 소유권(Ownership) 사슬이 필요하다.** `PossessedBy`가 `SetOwner(NewController)`를
  가장 먼저 하는 이유다 (`Pawn.cpp:673`). 소유자가 없으면 그 액터의 Client RPC를 어느 연결로
  보낼지 알 수 없다.

## 6. 빈도와 연관성 — "보낼 수 있다"와 "보낸다"는 다르다

- `NetUpdateFrequency` — 이 액터가 초당 몇 번 리플리케이션 후보가 되는지
- `NetDormancy` — 변화가 없는 액터를 잠재워 대역폭을 아낀다
- Relevancy — 너무 멀리 있는 액터는 아예 보내지 않는다
- `ForceNetUpdate()` — 다음 기회를 기다리지 않고 지금 보낸다.
  GAS의 `ForceReplication`도 이것을 호출한다 (`AbilitySystemComponent.cpp:1901`)

## 7. GAS는 이 위에 어떻게 얹히는가

- **ASC는 리플리케이트되는 컴포넌트**이고, 어트리뷰트는 AttributeSet의 리플리케이트 프로퍼티다.
- **GE 적용은 서버에서만 일어난다.** 클라이언트는 GE를 직접 적용하지 않고,
  그 결과인 어트리뷰트 값·태그·GameplayCue를 받는다.
- Replication Mode 세 가지는 **"ActiveGameplayEffects 목록을 누구에게 보낼 것인가"**를 정한다
  (`AbilitySystemComponent.h:81-89`, 실제 조건은 `GameplayEffect.cpp:5183-5201`).

| 모드 | GE 목록 리플리케이션 | 용도 |
|---|---|---|
| `Minimal` | `COND_Never` — 아예 보내지 않는다. 어트리뷰트·태그·큐만 전달 | AI. 남이 걸린 버프의 상세 내역까지 알 필요는 없다 |
| `Mixed` | `COND_OwnerOnly` — 소유 클라이언트에게만 | 플레이어. 자기 버프 목록과 남은 시간을 UI에 띄우고 예측에도 써야 한다 |
| `Full` | 모두에게 | 주로 싱글플레이 |

- 엔진 주석은 **소유자가 있는 ASC(즉 플레이어)에는 `Minimal`을 쓰지 말라**고 명시한다
  (`AbilitySystemComponent.h:83`).
- **예측(Prediction)**: 클라이언트가 서버 응답을 기다리면 공격이 늦게 나간다.
  GAS는 `LocalPredicted` 어빌리티와 PredictionKey로 클라이언트가 먼저 실행하고 서버가 확정하는
  구조를 제공한다. → 실제로 쓰게 될 때 별도 노트로 정리한다.

## 8. 현재 코드에 적용된 것

- **초기화 GE는 서버에서만 적용** — [`ABlademasterCharacter::BeginPlay`](../../Source/Blademaster/Characters/BlademasterCharacter.cpp)의
  `HasAuthority() && InitializeAttributesEffect` 체크. 클라이언트는 결과를 리플리케이션으로 받는다.
- **ASC는 생성자에서 `SetIsReplicated(true)`**, Replication Mode는 기본 `Minimal`로 두고
  `PossessedBy`에서 컨트롤러 종류에 따라 플레이어는 `Mixed`, AI는 `Minimal`로 정한다.
  → [모드별로 무엇이 전송되는지 정리한 표](#7-gas는-이-위에-어떻게-얹히는가)와 엔진 주석을 따른 선택.
- **`SetNetUpdateFrequency(100.f)` / `SetMinNetUpdateFrequency(2.f)`** — ASC를 PlayerState가 아니라
  Pawn에 두었기 때문에, Pawn의 갱신 빈도가 곧 어트리뷰트와 GameplayCue의 반영 속도가 된다.
  `ACharacter`의 기본값은 이동 스무딩 기준이라 명시적으로 올려 잡았다.
- **AttributeSet은 `REPNOTIFY_Always`** — [`UBlademasterAttributeSet::GetLifetimeReplicatedProps`](../../Source/Blademaster/AbilitySystem/BlademasterAttributeSet.cpp).
  값이 이전과 같아도 `OnRep`을 호출하게 해서 [프로퍼티는 최종 값만 보장한다](#4-동기화-수단-프로퍼티-리플리케이션)는 문제를 줄인다.
- **락온은 리플리케이트하지 않는다** — [`UBlademasterTargetingComponent`](../../Source/Blademaster/Combat/BlademasterTargetingComponent.cpp)는
  카메라와 입력에 관한 순수 로컬 상태라 서버가 알 필요가 없다.
  **무엇을 동기화하지 않을지 정하는 것도 설계의 절반이다.**

## 9. 자주 하는 실수

1. 클라이언트에서 상태를 바꾼다 → 다음 리플리케이션에 덮어써지고, 그동안 화면만 어긋난다.
2. 서버 전용 콜백(`PossessedBy`)에 클라이언트도 필요한 로직을 넣는다.
3. 순간적인 사건을 프로퍼티로 전달한다 → 중간 값이 유실되어 이펙트가 안 나온다.
4. 리플리케이션 도착 **순서**를 가정한다. `Controller`와 `PlayerState` 중 어느 쪽이 먼저 올지
   정해져 있지 않다. 엔진도 이 문제를 `OnRep_Controller` 안에서 따로 보정한다 (`Pawn.cpp:622-630`).
5. 타이밍이 중요한 기능을 만들면서 갱신 빈도를 확인하지 않는다.
6. Dedicated Server에서 돌면 안 되는 코드(UI, 입력, 카메라)를 권한 체크 없이 넣는다.

## 10. 검증 방법

- PIE 설정에서 Number of Players를 2로, Net Mode를 Listen Server로 둔다.
- 로그에 `[Server]` / `[Client]` 접두사를 붙이면 두 창의 로그를 구분할 수 있다
  (`HasAuthority()`나 `GetNetMode()`로 판별).
- `Net PktLag` 같은 콘솔 명령으로 지연을 흉내 내면, 지연이 있을 때만 드러나는 문제를 미리 볼 수 있다.

## 11. 엔진 소스 참고 (UE 5.8)

| 파일 | 위치 | 내용 |
|---|---|---|
| `Engine/Source/Runtime/Engine/Classes/Engine/EngineTypes.h` | 3582 | `ENetRole` 정의 |
| `Engine/Source/Runtime/Engine/Private/Pawn.cpp` | 673, 688-698 | `SetOwner`, `SetAutonomousProxy` |
| 〃 | 622-630 | 리플리케이션 순서를 보정하는 `OnRep_Controller` |
| `Engine/Plugins/Runtime/GameplayAbilities/.../Public/AbilitySystemComponent.h` | 81-89 | `EGameplayEffectReplicationMode` 정의와 주석 |
| `Engine/Plugins/Runtime/GameplayAbilities/.../Private/GameplayEffect.cpp` | 5183-5201 | 모드별 실제 리플리케이션 조건 |
| `Engine/Plugins/Runtime/GameplayAbilities/.../Private/AbilitySystemComponent.cpp` | 1901 | `ForceReplication` |
