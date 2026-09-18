# 012 — 루트모션과 Motion Warping

- 기준 엔진: UE 5.8

---

## 1. 한 문장 요약

루트모션은 **이동과 회전의 주도권을 코드에서 애니메이션으로 넘기는 것**이다.
그래서 루트모션 중에는 `CharacterMovement`의 가속·조향이 꺼지고, 이동량을 바꾸고 싶으면
**루트모션 값 자체를 수정**해야 한다. Motion Warping이 그 수정을 맡는 플러그인이다.

## 2. 루트모션이란

| | 보통의 이동 | 루트모션 |
|---|---|---|
| 캐릭터를 움직이는 주체 | `CharacterMovement` (입력·속도·가속) | 애니메이션의 루트 본 |
| 애니메이션의 역할 | 제자리에서 다리를 움직여 보여 준다 | 이동량 자체를 담고 있다 |
| 발 미끄러짐 | 속도와 애니메이션이 어긋나면 생긴다 | 없다 (이동량이 곧 애니메이션) |
| 이동 거리 조절 | 코드로 자유롭게 | **애니메이션에 고정** |

공격 모션에서 앞으로 나아가는 거리, 피격 모션에서 밀려나는 거리가 애니메이션에 들어 있다.
동작과 이동이 정확히 맞아떨어지는 대신, **거리를 상황에 맞게 바꾸기 어렵다.** 6절의 Motion Warping이 이 문제에서 출발한다.

## 3. 어떤 애니메이션의 루트모션을 쓸 것인가

AnimBP의 `Root Motion Mode`가 정한다 (`AnimEnums.h:28-42`).

| 모드 | 뜻 |
|---|---|
| `NoRootMotionExtraction` | 추출하지 않는다 |
| `IgnoreRootMotion` | 추출하지만 적용하지 않는다 |
| `RootMotionFromEverything` | 최종 포즈에 기여하는 **모든** 애니메이션에서 |
| `RootMotionFromMontagesOnly` | **몽타주에서만** |

엔진 주석이 네트워크 적합성을 직접 적어 두었다.
- `RootMotionFromEverything`: "네트워크 멀티플레이에 적합하지 않다"
- `RootMotionFromMontagesOnly`: "네트워크 멀티플레이에 적합하다"

이유는 몽타주가 **복제되는 단위**이기 때문이다(5절). 상태 기계가 고른 애니메이션은 기계마다 다를 수 있지만, 몽타주는 "무엇을 언제부터 재생 중인가"가 복제된다.

## 4. 루트모션 중 `CharacterMovement`가 하는 일과 하지 않는 일

이 노트에서 가장 중요한 부분이다. 루트모션이 재생되는 동안(`HasAnimRootMotion()`)의 동작이 평소와 다르다.

### 하지 않는 일

| 평소 | 루트모션 중 | 코드 |
|---|---|---|
| 입력으로 가속·감속을 계산해 `Velocity`를 만든다 | **계산하지 않는다.** `Velocity`를 루트모션 값으로 덮어쓴다 | `CharacterMovementComponent.cpp:2952-2960`, `:4514` |
| 이동 방향으로 몸을 돌린다(`bOrientRotationToMovement`), 컨트롤러 방향을 따라간다(`bUseControllerDesiredRotation`) | **`PhysicsRotation` 자체를 건너뛴다** | `:3044` |

**결과: 루트모션 중에는 스틱을 밀어도 방향이 바뀌지 않고 속도도 변하지 않는다.**

공격 중 방향 전환, 이동 보정 같은 것을 "이동 입력으로" 구현하려 하면 동작하지 않는 이유가 이것이다.

- 회전만 예외로 허용하려면 `bAllowPhysicsRotationDuringAnimRootMotion`을 켠다 (`CharacterMovementComponent.h:1082-1083`). 기본은 꺼져 있다.

### 하는 일

- 애니메이션의 **로컬** 루트모션 델타를 월드 공간으로 변환해 이동에 적용한다 (`:2957-2961`).
- 루트모션에 **회전**이 들어 있으면, 이동을 마친 뒤 그 회전을 캐릭터에 적용한다 (`:3049-3056`).
- 충돌 처리는 그대로 한다. 루트모션이 벽 쪽으로 밀어도 벽을 통과하지는 않는다.

즉 루트모션 중에는 **애니메이션이 속도와 회전의 주인**이고, `CharacterMovement`는 그 값을 월드로 옮겨 충돌을 처리하는 역할을 한다.

## 5. 네트워크에서의 루트모션

| 기계 | 동작 |
|---|---|
| 서버 | 최종 위치의 권한을 가진다 |
| 소유 클라이언트 | 자기가 재생한 몽타주의 루트모션으로 먼저 움직이고, 서버와 어긋나면 이동 보정을 받는다 |
| 다른 클라이언트 | 복제된 몽타주 정보로 같은 몽타주를 재생하며 루트모션을 흉내 낸다 (`SimulateRootMotion`, `CharacterMovementComponent.cpp:2106`) |

- 시뮬레이션 중인지를 `bWasSimulatingRootMotion`으로 추적하고, 끝나면 일반 이동 처리로 돌아간다 (`:2016-2082`).
- 그래서 **몽타주를 `ASC->PlayMontage`로 재생해야 한다**(010 §8). `AnimInstance`로 직접 틀면 복제되지 않아, 남의 화면에서는 그 캐릭터가 제자리에 서 있다.

## 6. Motion Warping

### 왜 필요한가

루트모션의 이동량은 애니메이션에 고정되어 있다.

- 적이 2m 앞인데 공격 모션이 1m만 전진하면 칼이 닿지 않는다.
- 락온 중인데 공격 모션이 정면으로만 나가면 대상을 빗나간다.

이동 입력으로 보정할 수도 없다(4절). 남은 방법은 **루트모션 값 자체를 수정하는 것**이고, 그것을 구조화한 것이 Motion Warping 플러그인이다.

### 동작 구조

```mermaid
graph LR
    A["애니메이션<br/>로컬 루트모션 델타"] --> B["MotionWarpingComponent<br/>구간이 열려 있으면 수정"]
    B --> C["월드 공간 변환"] --> D["CharacterMovement<br/>이동·회전 적용"]
```

1. `UMotionWarpingComponent`가 캐릭터의 **"로컬 루트모션을 월드로 변환하기 직전"** 지점에 연결된다
   (`MotionWarpingComponent.cpp:321`, 델리게이트 `WarpLocalRootMotionDelegate`).
2. **워프 타깃**은 이름이 붙은 목표(위치·회전)다. 코드가 갱신한다.
   `AddOrUpdateWarpTargetFromLocationAndRotation(이름, 위치, 회전)` (`MotionWarpingComponent.h:222`),
   컴포넌트나 트랜스폼으로 지정하는 함수도 있다 (`:186`, `:200`).
3. **몽타주에 배치한 Motion Warping 노티파이 구간**이 수정자(`URootMotionModifier`)를 만든다 (`AnimNotifyState_MotionWarping.h:34`).
4. 그 구간 동안에만 루트모션 델타가 목표에 맞게 수정된다.

### 노티파이 구간에서 정하는 것 (`RootMotionModifier.h:355-414`)

| 설정 | 뜻 |
|---|---|
| `WarpTargetName` | 어느 워프 타깃을 쓸 것인가 |
| `bWarpTranslation` | **이동**을 목표 위치에 맞게 보정할 것인가 |
| `bWarpRotation` | **회전**을 목표 방향에 맞게 보정할 것인가 |
| `RotationType` | 목표의 회전을 그대로 따를 것인가(`Default`), 목표를 바라볼 것인가(`Facing`) |
| `RotationMethod`, `WarpRotationTimeMultiplier` | 어떤 방식으로, 구간 중 언제까지 회전을 끝낼 것인가 |

이동과 회전을 **따로 켤 수 있다**는 점이 중요하다. "거리는 그대로 두고 방향만 맞추고 싶다"가 가능하다.

### 역할 분리

Motion Warping의 구조는 두 질문을 서로 다른 곳에서 답하게 만든다.

| 질문 | 답하는 곳 |
|---|---|
| **어디를 향할 것인가** | 코드가 갱신하는 워프 타깃 |
| **언제 실제로 보정할 것인가** | 몽타주의 노티파이 구간 |

그래서 "칼이 나가기 전까지만 방향을 틀 수 있다" 같은 타이밍 규칙을 **코드가 아니라 몽타주에서** 조정할 수 있다.

## 7. 현재 코드에 적용된 것

**설정**

- AnimBP의 Root Motion Mode는 `RootMotionFromMontagesOnly`다. 로코모션은 `CharacterMovement`가, 공격·피격·사망은 몽타주 루트모션이 담당한다.
- 공격·피격·사망 몽타주는 모두 루트모션 애니메이션으로 만들었다. 맞으면 모션만큼 밀려난다.

**공격 중 방향 전환** — [`ABlademasterCharacter`](../../Source/Blademaster/Characters/BlademasterCharacter.cpp)

```cpp
// Tick — 공격 중이면 매 틱 워프 타깃을 갱신한다. 위치는 현재 위치라 이동은 건드리지 않는다.
if (MotionWarpingComponent && AbilitySystemComponent
    && AbilitySystemComponent->HasMatchingGameplayTag(BlademasterGameplayTags::State_Attacking))
{
    MotionWarpingComponent->AddOrUpdateWarpTargetFromLocationAndRotation(
        AttackDirectionWarpTargetName, GetActorLocation(), GetAttackDirection());
}
```

| 항목 | 선택 |
|---|---|
| 보정 대상 | **회전만** 켠다. 이동 보정은 쓰지 않아 공격 거리는 애니메이션 그대로다 |
| 워프 타깃 위치 | 항상 현재 위치. 회전만 쓰므로 위치는 의미가 없다 |
| 갱신 시점 | 공격 중(`State.Attacking`) 매 틱 |
| 실제 회전 구간 | 몽타주에 배치한 Motion Warping 노티파이 |
| 방향 계산 | `GetAttackDirection()` — 락온 중이면 실시간 대상 방향, 아니면 콤보 시작 시 스틱 방향을 캐싱 |

**`Tick`이 락온 여부를 모른다는 점**

`Tick`은 `GetAttackDirection()` 결과를 그대로 넣기만 한다. 락온이냐 스틱이냐의 판단은 전부 그 가상 함수 안에 있다.
그래서 적(AI)이 같은 캐릭터 클래스를 쓰면서 자기 방식으로 방향만 재정의하면 이 구조를 그대로 쓸 수 있다.

**락온 회전과의 분리**

락온 중에는 평소 몸이 `ControlRotation`의 Yaw를 따라간다(`bUseControllerRotationYaw`). 공격 중에는 이것을 끄고 회전을 Motion Warping에 맡긴다.
카메라가 따라가는 `ControlRotation` 갱신은 공격 중에도 계속한다. 둘은 같은 값을 쓰지만 목적이 다르다(→ [006](006-rotation-and-coordinate-spaces.md)).

## 8. 자주 하는 실수

1. **루트모션 중에 이동 입력으로 방향을 바꾸려 한다.** `PhysicsRotation`이 꺼져 있어 아무 일도 일어나지 않는다(4절).
2. **`bOrientRotationToMovement`가 공격 중에도 동작할 거라고 가정한다.**
3. **몽타주를 `AnimInstance`로 직접 재생한다.** 복제되지 않아 다른 클라이언트에서는 캐릭터가 이동하지 않는다.
4. **제자리(InPlace) 버전 애니메이션으로 몽타주를 만든다.** 루트모션이 없어 전진하지 않는다. 같은 이름의 두 버전이 들어 있는 애님 팩이 많다.
5. **루트모션이 중간에 끊기면 남은 이동도 일어날 거라고 가정한다.** 몽타주가 멈추면 이동도 거기서 끝난다.
6. **Motion Warping에서 회전만 쓰고 싶은데 이동 보정까지 켠다.** 공격 거리가 목표에 맞춰 늘어나거나 줄어든다.
7. **워프 타깃만 갱신하고 몽타주에 노티파이 구간을 두지 않는다.** 목표는 있지만 보정할 구간이 없어 아무 일도 일어나지 않는다.
8. **`RootMotionFromEverything`을 멀티플레이 프로젝트에 쓴다.** 엔진 주석이 적합하지 않다고 명시한다(3절).

## 9. 자가 점검 질문

1. 루트모션은 무엇을 어디에서 어디로 옮기는가? 발 미끄러짐이 없는 이유는?
2. `RootMotionFromMontagesOnly`가 네트워크에 적합하다고 하는 이유는?
3. 루트모션 중에 `CharacterMovement`가 **하지 않는** 일 두 가지는?
4. 루트모션 중에 `CharacterMovement`가 여전히 **하는** 일은?
5. 공격 중 방향 전환을 이동 입력으로 구현할 수 없는 이유는?
6. 다른 클라이언트에서 루트모션이 재현되는 경로는? 몽타주를 ASC로 재생해야 하는 이유는?
7. Motion Warping은 루트모션의 어느 시점에 끼어드는가?
8. 워프 타깃과 노티파이 구간은 각각 무엇을 정하나?
9. 이 프로젝트가 이동 보정을 끄고 회전만 켠 이유는?
10. 캐릭터 `Tick`이 락온 여부를 모르게 만든 이유는?

## 10. 엔진 소스 참고 (UE 5.8)

| 파일 | 위치 | 내용 |
|---|---|---|
| `Engine/Source/Runtime/Engine/Classes/Animation/AnimEnums.h` | 28-42 | `ERootMotionMode`와 네트워크 적합성 주석 |
| `.../Classes/GameFramework/CharacterMovementComponent.h` | 1082-1083 | `bAllowPhysicsRotationDuringAnimRootMotion` |
| `.../Private/Components/CharacterMovementComponent.cpp` | 2952-2961 | 루트모션이 `Velocity`를 덮어쓰고 월드 공간으로 변환되는 지점 |
| 〃 | 3044-3056 | 루트모션 중 `PhysicsRotation` 건너뛰기, 루트모션 회전 적용 |
| 〃 | 4514 | `ConstrainAnimRootMotionVelocity` — 애님 루트모션이 다른 루트모션 소스보다 우선 |
| 〃 | 2106, 2016-2082 | `SimulateRootMotion`, `bWasSimulatingRootMotion` |
| `Engine/Plugins/Animation/MotionWarping/.../MotionWarpingComponent.cpp` | 321 | 루트모션 변환 직전 델리게이트에 연결 |
| `.../Public/MotionWarpingComponent.h` | 178-222 | 워프 타깃 추가·갱신 함수들 |
| `.../Public/AnimNotifyState_MotionWarping.h` | 25-43 | 노티파이가 수정자를 만들고 활성·갱신·해제하는 경로 |
| `.../Public/RootMotionModifier.h` | 300-414 | `EMotionWarpRotationType`, `bWarpTranslation` / `bWarpRotation`, 회전 방식 설정 |
