# 001 — 액터 생애주기와 초기화 순서

- 기준 엔진: UE 5.8

---

## 1. 왜 알아야 하는가

엔진은 내 코드를 **정해진 시점에** 부르고, 시점마다 준비된 것과 아직 없는 것이 다르다.
초기화 버그는 거의 항상 둘 중 하나다.

- **아직 없는 것에 접근했다** — 예: 컨트롤러가 붙기 전에 컨트롤러를 찾는다.
- **한 번만 해야 할 일을 여러 번 했다** — 예: 빙의할 때마다 초기화 GE를 적용한다.

## 2. 액터의 일생

### 2.1 생성자 — 클래스의 "틀"을 만든다

- 생성자는 게임 중에만 불리지 않는다. 엔진이 클래스를 로드할 때 **[CDO](002-class-default-object.md)를 만들면서도** 실행된다.
- **Blueprint에서 지정한 값은 생성자가 끝난 뒤에 적용된다.** 생성자 안에서 BP로 지정하는
  프로퍼티를 읽으면 항상 C++ 기본값(대개 null)이다.
- 생성자에서 하는 일: 컴포넌트 생성(`CreateDefaultSubobject`), 기본값 설정.
- 생성자에서 하지 않는 일: 월드·다른 액터·네트워크 접근, BP 값에 의존하는 로직.

### 2.2 스폰 파이프라인

`AActor::PostSpawnInitialize` → `FinishSpawning` → `PostActorConstruction` 순으로 진행된다.

```
SpawnActor
 → PostActorCreated
 → OnConstruction (Construction Script)
 → 컴포넌트 등록 (OnRegister)       ← ASC가 AbilityActorInfo를 할당하는 곳
 → PreInitializeComponents
 → InitializeComponents
 → PostInitializeComponents         ← 여기까지는 "액터 내부"만 준비됨
 → BeginPlay (월드가 이미 시작됐을 때만)
```

- AnimInstance 초기화(`NativeInitializeAnimation`)도 메시 컴포넌트가 등록될 때 일어나므로
  BeginPlay보다 먼저다. 이 시점에는 Pawn의 컨트롤러가 없을 수 있다.

### 2.3 BeginPlay — 액터당 한 번

- 조건은 `World->HasBegunPlay()`다 (`Actor.cpp:4509`).
  - 월드가 시작되기 **전에** 존재하던 액터(레벨 배치 액터, 시작 직전에 스폰된 액터)는
    월드가 시작될 때 일괄로 BeginPlay를 받는다.
  - 월드가 시작된 **뒤에** 스폰된 액터는 스폰 도중에 바로 BeginPlay를 받는다.
- 이 차이 때문에 [BeginPlay와 PossessedBy의 순서가 고정되지 않는다](#4-beginplay와-possessedby의-순서는-고정되어-있지-않다).

### 2.4 빙의 — 서버에서만 일어나는 이벤트

```
GameMode::PostLogin
 → HandleStartingNewPlayer
 → RestartPlayer
 → SpawnDefaultPawnFor  (= SpawnActor, 2.2가 전부 여기서 실행된다)
 → Controller::Possess
 → Pawn::PossessedBy                ← 서버 전용
 → NotifyControllerChanged
```

- 클라이언트에서는 `PossessedBy`가 불리지 않는다. 대신 `Controller`가 리플리케이트되어
  `OnRep_Controller`가 오고, 거기서 `NotifyControllerChanged`가 불린다 (`Pawn.cpp:615-644`).
- 빙의는 **여러 번 일어날 수 있다** (빙의 해제 후 재빙의, 다른 컨트롤러로 교체).

### 2.5 끝

`UnPossessed` → `EndPlay` → `Destroyed`. 적이 죽고 사라지기 시작하는 M3에서 다시 다룬다.

## 3. 서버와 클라이언트에서 불리는 것

| 함수 | 서버 | 클라이언트 | 횟수 |
|---|---|---|---|
| 생성자 | O | O | 인스턴스당 1회 (+ CDO) |
| `PostInitializeComponents` | O | O | 1회 |
| `BeginPlay` | O | O — 리플리케이트된 값이 도착한 뒤로 미뤄진다 (`Actor.cpp:4444`) | 1회 |
| `PossessedBy` | O | **X** | 빙의할 때마다 |
| `OnRep_Controller` | X | O | 컨트롤러가 바뀔 때마다 |
| `NotifyControllerChanged` | O | O | 컨트롤러가 바뀔 때마다 |

## 4. BeginPlay와 PossessedBy의 순서는 고정되어 있지 않다

같은 클래스, 같은 코드라도 **언제 스폰되느냐에 따라 순서가 뒤집힌다.**

| 상황 | 순서 | 근거 |
|---|---|---|
| PIE 시작 시 호스트 플레이어 | `PossessedBy` → `BeginPlay` | 플레이어 생성(`GameInstance.cpp:538`)이 월드 `BeginPlay()`(`:566`)보다 먼저다. 플레이어 생성 안에서 `PostLogin`(`LevelActor.cpp:1111`)부터 빙의까지 끝난다 |
| 월드 시작 후 스폰되는 Pawn (리스폰, 나중에 접속한 플레이어) | `BeginPlay` → `PossessedBy` | 스폰 도중 BeginPlay가 실행되고(`Actor.cpp:4509`), `SpawnActor`가 반환된 뒤에야 `Possess`가 불린다 (`GameModeBase.cpp:1295, 1364`) |

따라서 **"BeginPlay에서는 컨트롤러가 있다"는 가정은 할 수 없다.**

## 5. 초기화 코드를 어디에 둘지 정하는 네 가지 질문

1. **몇 번 실행되어야 하나?**
   - 액터당 1회 → `BeginPlay` / `PostInitializeComponents`
   - 컨트롤러가 바뀔 때마다 → `PossessedBy` / `NotifyControllerChanged`
2. **무엇이 준비되어 있어야 하나?**
   - BP에서 지정한 값 → 생성자는 안 된다
   - 컨트롤러 → 빙의 이후
   - 서버가 보낸 값 → 클라이언트에서는 `OnRep`
3. **어디서 실행되나?**
   - 서버 전용 이벤트(`PossessedBy`)에 클라이언트도 필요한 로직을 넣으면 클라이언트에서는 영영 실행되지 않는다.
4. **순서가 보장되나?**
   - 보장되지 않으면 두 경로 모두에서 처리하되, 여러 번 실행해도 결과가 같게(멱등하게) 만든다.

## 6. 프로젝트 사례

### 재빙의 시 초기화 GE 중복 적용 (커밋 `8eff398`)

- 이전에는 초기화 GE를 `PossessedBy`에서 적용했다. `PossessedBy`는 빙의할 때마다 불리므로
  재빙의하면 Health·Posture가 최대치로 다시 초기화된다. → 질문 1을 틀린 경우.
- 초기화 GE와 `InitAbilityActorInfo`를 `BeginPlay`로 옮겨 "액터당 1회"를 보장했다.
- 컨트롤러 종류에 따라 달라지는 `SetReplicationMode`만 `PossessedBy`에 남겼다.
  → 작업의 성격(1회인가, 컨트롤러마다인가)에 따라 위치를 나눈 것.
- 코드: [`ABlademasterCharacter::BeginPlay` / `PossessedBy`](../../Source/Blademaster/Characters/BlademasterCharacter.cpp)

### 이미 맞게 된 부분

- **입력 매핑 등록** — [`ABlademasterPlayerCharacter::NotifyControllerChanged`](../../Source/Blademaster/Characters/BlademasterPlayerCharacter.cpp)에서
  `IMC_Default`를 추가한다. 서버·클라이언트 모두에서 불리고 재빙의에도 따라간다.
- **AnimInstance** — [`UBlademasterAnimInstance::NativeUpdateAnimation`](../../Source/Blademaster/Animation/BlademasterAnimInstance.cpp)은
  초기화 시점에 Pawn이 없을 수 있어서 업데이트 때 다시 조회하고, 컨트롤러가 없으면 각도를 0으로 둔다.
  순서를 가정하지 않는 방어 코드다.
- **락온 컴포넌트** — [`UBlademasterTargetingComponent`](../../Source/Blademaster/Combat/BlademasterTargetingComponent.cpp)는
  생성자에서 틱을 "가능하지만 꺼진 상태"로 두고(틀), 락온할 때만 켠다(인스턴스 상태).

## 7. 엔진 소스 참고 (UE 5.8)

| 파일 | 위치 | 내용 |
|---|---|---|
| `Engine/Source/Runtime/Engine/Private/Actor.cpp` | `PostActorConstruction` (4430~) | 컴포넌트 초기화 → `PostInitializeComponents` → BeginPlay 조건 |
| 〃 | 4444 | 동적 스폰된 리플리케이트 액터는 클라이언트에서 BeginPlay를 미룬다 |
| `Engine/Source/Runtime/Engine/Private/GameInstance.cpp` | 538, 566 | PIE: 플레이어 생성 → 월드 BeginPlay |
| `Engine/Source/Runtime/Engine/Private/LevelActor.cpp` | 1111 | `SpawnPlayActor` 안에서 `PostLogin` |
| `Engine/Source/Runtime/Engine/Private/GameModeBase.cpp` | 1068, 1295, 1364 | `HandleStartingNewPlayer` → Pawn 스폰 → `Possess` |
| `Engine/Source/Runtime/Engine/Private/Pawn.cpp` | 615, 671, 740 | `OnRep_Controller`, `PossessedBy`, `NotifyControllerChanged` |
| `Engine/Plugins/Runtime/GameplayAbilities/.../AbilitySystemComponent.cpp` | 192 | `OnRegister`에서 `AbilityActorInfo` 할당 |
