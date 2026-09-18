# 009 — 게임플레이 태그 심화

- 기준 엔진: UE 5.8 (Iris를 쓰지 않는 기본 네트워크 설정)

---

## 1. 한 문장 요약

게임플레이 태그는 **미리 등록된 계층형 이름**이고, ASC는 태그마다 **개수**를 세어 들고 있다.
태그를 붙이는 쪽은 개수와 짝을 책임지고, 태그를 복제할지는 **태그를 붙일 때 고르는 옵션**으로 정한다.

태그의 쓰임새(상태 표시, 활성화 제어, 차단·취소, 분류)는 [004 §5](004-gas-overview.md#5-태그의-네-가지-쓰임)에 있다.
이 노트는 그 쓰임새가 **어떻게 동작하는지**를 다룬다.

## 2. 태그의 정체: 문자열이 아니라 등록된 이름

`FGameplayTag`는 내부적으로 `FName` 하나다. 비교는 이름이 같은지로 한다 (`GameplayTagContainer.h:41-73`).
다만 **아무 이름이나 태그가 될 수는 없다.** 태그 매니저(`UGameplayTagsManager`)에 등록된 이름만 유효한 태그다.

**등록 방법 두 가지**

| 방법 | 어디에 | 특징 |
|---|---|---|
| 설정 | `Config/DefaultGameplayTags.ini`, 데이터 테이블, 에디터의 태그 관리 창 | 코드 없이 추가. 기획자가 다루기 쉽다 |
| 네이티브 | C++ `.cpp`에서 `UE_DEFINE_GAMEPLAY_TAG` 계열 매크로 | 모듈이 로드될 때 등록된다. C++ 변수로 참조한다 |

**문자열로 찾기 — `RequestGameplayTag`**

```cpp
FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName("Attack.Window"));
```

- 등록되지 않은 이름이면 빈 태그를 돌려준다. `ErrorIfNotFound`가 기본 `true`라 `ensure`가 뜬다 (`GameplayTagContainer.h:57`).
- 문제는 **오타가 컴파일 단계에서 걸리지 않는다**는 점이다. 실행해서 그 줄에 도달해야 알 수 있다.
- 태그 이름을 바꾸면 문자열을 쓴 곳을 모두 찾아 고쳐야 한다.

**네이티브 태그** (`NativeGameplayTags.h:31-41`)

```cpp
// 헤더: 다른 파일이 쓸 수 있게 선언
UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Attacking);

// .cpp: 정의하면서 등록
UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Attacking, "State.Attacking", "설명");
```

- 코드에서는 변수(`BlademasterGameplayTags::State_Attacking`)로 쓴다. **변수 이름을 틀리면 컴파일이 실패한다.**
- 정의 매크로는 `.cpp`에서만 쓸 수 있게 `static_assert`로 막혀 있다. 헤더에 정의하면 여러 번 등록되기 때문이다.
- 태그 문자열 자체는 한 곳(`.cpp`)에만 적는다.

## 3. 계층과 매칭

태그 `A.B.C`는 자기 자신과 함께 부모 `A.B`, `A`를 **암묵적으로 가진다.**
매칭 함수는 이 부모를 포함해서 보느냐(기본)와 정확히 같은 것만 보느냐(`Exact`)로 나뉜다.

**방향이 중요하다: 자식은 부모에 매칭되지만, 부모는 자식에 매칭되지 않는다.**

| 식 | 결과 | 이유 |
|---|---|---|
| `"A.1".MatchesTag("A")` | `true` | `A.1`은 부모 `A`를 가진다 |
| `"A".MatchesTag("A.1")` | `false` | `A`는 자식을 가지지 않는다 |
| `"A.1".MatchesTagExact("A")` | `false` | 정확히 같지 않다 |

엔진 주석에 이 예가 그대로 있다 (`GameplayTagContainer.h:78-107`).

**컨테이너 함수** (`GameplayTagContainer.h:299-420`)

| 함수 | 의미 | 예 |
|---|---|---|
| `HasTag(T)` | 컨테이너(와 그 부모들)에 T가 있나 | `{"A.1"}.HasTag("A")` → `true` |
| `HasTagExact(T)` | 컨테이너에 T가 정확히 있나 | `{"A.1"}.HasTagExact("A")` → `false` |
| `HasAny(C)` | C 중 하나라도 있나 | `{"A.1"}.HasAny({"A","B"})` → `true` |
| `HasAll(C)` | C가 전부 있나 | `{"A.1","B.1"}.HasAll({"A","B"})` → `true` |

- **부모 확장은 호출한 쪽(컨테이너)에만 적용된다.** 인자로 넘긴 태그는 확장하지 않는다.
  `{"A"}.HasTag("A.1")`은 `false`다.
- **빈 인자의 결과가 다르다.** `HasAny({})`는 `false`, `HasAll({})`는 `true`다. "조건이 없으면 전부 만족"이라는 뜻이다.

**태그 쿼리 — `FGameplayTagQuery`**

"A는 있고 B는 없고, C나 D 중 하나는 있어야 한다" 같은 조합 조건을 데이터로 표현하는 구조체다.
`Has*` 함수 여러 개를 코드로 엮는 대신 에셋에서 조건을 편집할 때 쓴다. M1에서는 쓰지 않았다.

## 4. ASC는 태그를 어떻게 들고 있나

ASC의 태그는 `FGameplayTagCountContainer` 하나에 들어 있다 (`AbilitySystemComponent.h:1874-1876`).
이 구조체는 두 가지를 따로 센다 (`GameplayEffectTypes.h:1102-1400`).

| 필드 | 무엇을 세나 |
|---|---|
| `Items` | **명시적으로 붙인** 태그마다 개수 (그리고 복제 옵션, §6) |
| `GameplayTagCountMap` | 명시적 태그 **와 그 부모들**의 개수 |

**예: 세 번 붙이면**

```cpp
ASC->AddLooseGameplayTag(Tag_A_B);   // A.B
ASC->AddLooseGameplayTag(Tag_A_B);   // A.B 한 번 더
ASC->AddLooseGameplayTag(Tag_A_C);   // A.C
```

| 태그 | 명시적 개수 (`GetExplicitTagCount`) | 계층 개수 (`GetTagCount`) |
|---|---|---|
| `A.B` | 2 | 2 |
| `A.C` | 1 | 1 |
| `A` | 0 | **3** |

- 개수를 늘릴 때 태그 자신과 **모든 부모**의 개수에 같은 값을 더한다 (`GameplayEffectTypes.cpp:848-870`).
- `HasMatchingGameplayTag(T)`는 계층 개수가 0보다 큰지 본다 (`GameplayEffectTypes.h:1113-1116`).
  그래서 `A.B`만 붙어 있어도 `HasMatchingGameplayTag(A)`는 `true`다.

**같은 태그를 두 번 붙이고 한 번 떼면**

개수가 2 → 1이 될 뿐 **태그는 여전히 붙어 있다.** 0이 되어야 사라진다.

- 태그가 붙은 이유가 두 개(예: 두 효과가 각각 같은 상태를 준다)면 하나가 끝나도 상태가 유지된다. 이게 개수를 세는 이유다.
- 반대로 붙인 횟수와 뗀 횟수가 어긋나면 태그가 영원히 남는다.
- 없는 태그를 떼려고 하면 음수가 되지 않고 무시된다. 로그는 `Verbose`라 평소에는 보이지 않는다 (`GameplayEffectTypes.cpp:838-842`).

## 5. 태그가 붙는 출처 세 가지와 수명 책임

모든 출처가 같은 `FGameplayTagCountContainer`의 개수를 올리고 내린다. 차이는 **누가 짝을 맞추느냐**다.

| 출처 | 붙는 시점 | 떼는 책임 | 짝이 어긋날 위험 |
|---|---|---|---|
| GE의 부여 태그 | 지속형 GE가 적용될 때 | **엔진** — GE가 사라질 때 (`GameplayEffect.cpp:4971-4986`) | 낮음 |
| 어빌리티의 `ActivationOwnedTags` | `PreActivate` | **엔진** — `EndAbility` ([008 §5](008-ability-lifecycle-and-tasks.md#5-한-번-실행되는-흐름)) | 낮음 (어빌리티가 끝나기만 하면) |
| Loose 태그 | `AddLooseGameplayTag` | **붙인 코드** — `RemoveLooseGameplayTag` | **높음** |

"Loose"는 **아무 수명에도 묶여 있지 않다**는 뜻이다. 편리하지만, 붙이는 코드와 떼는 코드가 서로 다른 곳에 있으면
모든 경로(중단, 취소, 반복 호출)에서 짝이 맞는지 직접 보장해야 한다.

상태의 수명이 GE나 어빌리티의 수명과 같다면, Loose 대신 그쪽에 태그를 맡기는 편이 안전하다.

## 6. 태그 복제: 기계마다 태그 상태가 다를 수 있다

### 무엇이 복제되나

`FGameplayTagCountContainer`는 ASC의 복제 프로퍼티다 (`AbilitySystemComponent.cpp:1858`).
하지만 전부 보내지 않는다. **태그 항목마다 붙일 때 정한 복제 옵션**(`EGameplayTagReplicationState`)을 들고 있고,
그 옵션에 따라 보낼지, 누구에게, 개수까지 보낼지가 정해진다 (`GameplayEffectTypes.cpp:528-676`).

| 옵션 | 소유 클라이언트가 받는 것 | 다른 클라이언트가 받는 것 |
|---|---|---|
| `None` | 없음 | 없음 |
| `SimulatedTagOnly` | 없음 | 태그 (개수는 1) |
| `TagOnly` | 태그 (개수는 1) | 태그 (개수는 1) |
| `CountToOwner` | 태그 **와 개수** | 태그 (개수는 1) |
| `TagAndCountToAll` | 태그와 개수 | 태그와 개수 |

정의와 주석: `GameplayEffectTypes.h:1050-1057`. 개수를 실을지 판단하는 식: `GameplayEffectTypes.cpp:603`.

> 예전에는 `MinimalReplicationTags`, `ReplicatedLooseTags`라는 별도 컨테이너로 복제했다.
> 둘 다 5.7에서 deprecated 되었고, 위의 항목별 옵션 방식으로 대체되었다 (`AbilitySystemComponent.h:1921-1943`).
> 인터넷 자료의 상당수가 예전 방식 기준이니 주의한다.

### 출처별로 어떤 옵션이 쓰이나

| 출처 | 옵션 | 근거 |
|---|---|---|
| `ActivationOwnedTags` | `CountToOwner` (프로젝트 설정 `ReplicateActivationOwnedTags` 기본값 `true`) | `GameplayAbility.cpp:870, 990`, `GameplayAbilitiesDeveloperSettings.h:76` |
| GE 부여 태그, 복제 모드 `Minimal`·`Mixed` | `SimulatedTagOnly` | `GameplayEffect.cpp:4658-4665` |
| GE 부여 태그, 복제 모드 `Full` | `None` | 〃 |
| Loose 태그 (C++ 기본 인자) | `None` | `AbilitySystemComponent.h:656` |
| Loose 태그 (블루프린트 노드의 `bShouldReplicate`) | `CountToOwner` | `AbilitySystemBlueprintLibrary.cpp:1352` |

GE 태그가 `SimulatedTagOnly`인 이유: GE를 가진 기계는 GE를 통해 태그를 스스로 붙인다.
서버는 GE를 직접 적용하고, `Mixed`의 소유 클라이언트는 GE를 복제받는다.
GE를 받지 못하는 **다른 클라이언트에게만** 태그를 보내면 된다.
`Minimal`은 엔진 주석대로 소유 클라이언트가 있는 ASC에는 맞지 않고(`AbilitySystemComponent.h:82`) AI처럼 소유 클라이언트가 없는 경우에 쓴다.
`Full`에서는 모두가 GE를 받으므로 태그를 따로 보낼 필요가 없다. 복제 모드는 [003 §7](003-authority-and-replication.md#7-gas는-이-위에-어떻게-얹히는가) 참고.

### 받는 쪽에서 일어나는 일 — 복제된 값이 로컬 값을 덮는다

복제는 받는 기계의 **같은 컨테이너**에 적용된다. 로컬에서 붙인 태그와 따로 보관되지 않는다
(`GameplayEffectTypes.cpp:679-735`).

- **추가·변경:** 받은 태그의 개수를 받은 값으로 **맞춘다**(`SetTagCount`). 개수를 받지 않는 옵션이면 1로 맞춘다.
- **제거:** 받은 태그의 개수를 **전부** 없앤다. 로컬에서 따로 붙여 둔 몫도 함께 사라진다.
- 클라이언트가 `TagOnly` 태그를 로컬로 붙이면 개수가 1로 제한된다 (`:812-816`, `:864-867`).
  서버가 보낼 값(1)과 어긋나지 않게 하려는 처리다.
- 같은 태그를 서로 다른 옵션으로 붙이면 더 높은 옵션으로 올라가고 경고가 남는다 (`:792-808`).

그래서 **한 태그는 한 가지 방식으로만 관리**하는 게 원칙이다.
"서버가 복제하는 태그"와 "각 기계가 로컬로 붙이는 태그"를 같은 이름으로 섞으면 서로 덮어쓴다.

### 결론: 기계마다 태그가 다를 수 있다

- `None` 태그는 **각 기계가 각자 붙이고 뗀다.** 같은 몽타주를 재생해도 기계마다 시점이 조금씩 다를 수 있다.
- 복제되는 태그도 네트워크 지연만큼 늦게 도착한다.
- 따라서 "이 태그를 보고 판단하는 코드가 **어느 기계에서** 실행되는가"를 항상 같이 생각해야 한다.
  서버의 판단과 클라이언트의 판단이 다른 태그 상태를 볼 수 있다. 예측과의 관계는 010에서 다룬다.

## 7. 태그 변화 감지

**델리게이트 등록 — `RegisterGameplayTagEvent`** (`AbilitySystemComponent.h:720`)

```cpp
ASC->RegisterGameplayTagEvent(Tag, EGameplayTagEventType::NewOrRemoved)
    .AddUObject(this, &UMyClass::OnTagChanged);   // (FGameplayTag, int32 NewCount)
```

| 모드 | 언제 호출되나 |
|---|---|
| `NewOrRemoved` (기본) | 개수가 **0에서 생기거나 0이 될 때**만 |
| `AnyCountChange` | 개수가 바뀔 때마다 |

정의: `GameplayEffectTypes.h:1032-1042`.

- **부모 태그에 등록해도 된다.** 개수 변화는 부모에게도 전파되므로 `Attack.Window`에 등록하면 하위 구간 태그 중 첫 번째가 붙을 때 호출된다.
- 등록한 쪽이 먼저 사라지면 해제해야 한다. 델리게이트 핸들을 들고 있다가 `Remove`한다.

**감지 수단 비교**

| 수단 | 어울리는 곳 | 수명 관리 |
|---|---|---|
| `RegisterGameplayTagEvent` | 컴포넌트·캐릭터처럼 오래 사는 객체 | 직접 해제 |
| 태그 대기 태스크 (`WaitGameplayTagAdded` 등) | 실행 중인 어빌리티 안에서 | 어빌리티가 끝나면 자동 정리 ([008 §7](008-ability-lifecycle-and-tasks.md#7-어빌리티-태스크-왜-필요한가)) |
| 어빌리티 트리거 (`OwnedTagAdded`/`OwnedTagPresent`) | "이 태그가 붙으면 이 행동을 시작한다" | 엔진이 관리 ([008 §4](008-ability-lifecycle-and-tasks.md#4-발동-경로-세-가지)) |
| 매 틱 `HasMatchingGameplayTag` | 어차피 매 틱 도는 코드 | 없음 |

## 8. 태그로 어빌리티를 제어하는 동작

검사는 `CanActivateAbility` 안의 `DoesAbilitySatisfyTagRequirements`에서 한다 (`GameplayAbility.cpp:349-440`).
**무엇과 무엇을 비교하는지**, 그리고 **어느 쪽이 부모 확장되는지**가 핵심이다.

| 어빌리티 프로퍼티 | 비교 대상 | 검사식 | 위치 |
|---|---|---|---|
| `ActivationRequiredTags` | **소유자**가 가진 태그 | 소유자 태그`.HasAll(Required)` | `:419` |
| `ActivationBlockedTags` | 소유자가 가진 태그 | 소유자 태그`.HasAny(Blocked)` | `:408` |
| `BlockAbilitiesWithTag` | 이후 발동하려는 **다른 어빌리티의 `AssetTags`** | 그 어빌리티 `AssetTags.HasAny(차단 목록)` | `:407`, ASC `:1448-1452` |
| `CancelAbilitiesWithTag` | 지금 실행 중인 **다른 어빌리티의 `AssetTags`** | 그 어빌리티 `AssetTags.HasAny(취소 목록)` | ASC `:1329-1347` |
| `SourceRequired/BlockedTags`, `TargetRequired/BlockedTags` | 이벤트 데이터의 `InstigatorTags`/`TargetTags` | 위와 같은 방식 | `:409-426` |

(ASC 위치는 `AbilitySystemComponent_Abilities.cpp`)

**앞의 두 줄과 뒤의 두 줄은 보는 대상이 다르다.**

- `ActivationRequired/BlockedTags`는 "**캐릭터가 지금 어떤 상태인가**"를 본다. 예: `State.Dead`면 공격 불가.
- `BlockAbilitiesWithTag`·`CancelAbilitiesWithTag`는 "**어떤 종류의 어빌리티인가**"를 본다. 예: `Ability.Action` 계열은 막는다.

**부모 확장은 `AssetTags` 쪽에 일어난다**

- 차단 목록에 `Ability.Action`을 넣으면 `AssetTags`가 `Ability.Action.Attack`인 어빌리티가 막힌다. 자식이 부모에 매칭되기 때문이다.
- 반대로 차단 목록에 `Ability.Action.Attack`을 넣어도 `AssetTags`가 `Ability.Action`뿐인 어빌리티는 막히지 않는다.
- 그래서 **분류용 태그는 넓게(부모) 막고, 어빌리티에는 좁게(자식) 붙이는** 구성이 확장에 유리하다.

**차단도 개수로 관리된다**

- 차단 목록은 별도의 `FGameplayTagCountContainer`(`BlockedAbilityTags`)다 (`AbilitySystemComponent.h:1865-1866`).
- 어빌리티의 `BlockAbilitiesWithTag`는 `PreActivate`에서 +1, `EndAbility`에서 −1 된다 (엔진이 짝을 맞춘다).
- 코드에서 `ASC->BlockAbilitiesWithTags` / `UnBlockAbilitiesWithTags`를 직접 부르면 **짝은 호출한 코드의 책임**이다 (`:1454-1462`).
  Loose 태그와 같은 문제를 가진다.
- `BlockedAbilityTags`는 복제 프로퍼티가 아니다. 각 기계가 자기 어빌리티 실행에 따라 따로 가진다.

## 9. 태그로 설계할 때의 판단

태그는 "여러 곳이 이름으로 상태를 묻는" 문제에 맞는 도구다. 모든 상태를 태그로 만들 필요는 없다.

**태그가 맞는 경우**

| 조건 | 이유 |
|---|---|
| **여러 시스템이 조회한다** | 조회하는 쪽이 상태를 가진 객체를 몰라도 된다. ASC 하나에만 물으면 된다 |
| **에디터 데이터로 조건을 지정한다** | `ActivationBlockedTags` 같은 프로퍼티에 태그를 고르는 것만으로 규칙을 바꿀 수 있다. 코드 수정 없음 |
| **계층으로 분류해 한꺼번에 다룬다** | 부모 태그 하나로 앞으로 생길 자식들까지 포함한다 |
| **GAS 기능과 연결된다** | 트리거, 차단·취소, GE 조건, 복제 옵션을 그대로 쓸 수 있다 |

**태그가 맞지 않는 경우**

| 조건 | 대안 |
|---|---|
| **값이나 데이터를 함께 실어야 한다** (몇 타째, 남은 시간) | 멤버 변수, 어트리뷰트, 이벤트 페이로드 |
| **한 객체 안에서만 쓰는 상태다** | 멤버 변수 (`bool`) |
| **"일어났다"는 순간을 알려야 한다** | 델리게이트, 게임플레이 이벤트 |
| **물리 질의에서 걸러내야 한다** | 충돌 채널·프로필 — 질의 자체에서 빠지므로 결과를 받는 코드가 태그를 검사할 필요가 없다 |

판단할 때 던질 질문:

1. 이 상태를 **누가** 조회하나? 한 곳이면 태그가 과하다.
2. 이 상태의 **수명은 무엇과 같나?** GE나 어빌리티와 같다면 그쪽에 맡긴다(§5).
3. **다른 기계**도 이 상태를 알아야 하나? 그렇다면 어떤 복제 옵션인가(§6).
4. 조건을 **데이터로** 바꿀 일이 있나?

## 10. 현재 코드에 적용된 것

**정의** — [`BlademasterGameplayTags.h`](../../Source/Blademaster/BlademasterGameplayTags.h) / [`.cpp`](../../Source/Blademaster/BlademasterGameplayTags.cpp)

- 모든 태그가 네이티브 태그이고, `BlademasterGameplayTags` 네임스페이스로 묶여 있다.
- 태그에는 프로젝트 접두사를 붙이지 않고 계층으로 나눈다.

```
InputTag.Attack
Ability.Action.Attack          Ability.Reaction.Hit
State.Attacking   State.Dying   State.Dead
Attack.Window  (부모)
Attack.Window.Input   Attack.Window.Combo   Attack.Window.Cancel
GameplayEvent.WeaponHit
SetByCaller.Damage.Health   SetByCaller.Damage.Posture
```

**출처와 복제 옵션별로 보면**

| 태그 | 출처 | 복제 옵션 | 누가 보나 |
|---|---|---|---|
| `State.Attacking` | 콤보 어빌리티의 `ActivationOwnedTags` | `CountToOwner` (엔진 기본) | 캐릭터 `Tick`의 워프 타깃 갱신, 플레이어의 락온 회전 |
| `Attack.Window.*` | 노티파이 스테이트의 Loose 태그 | `None` | 콤보 어빌리티(선입력·이어가기), 디버그 표시 |
| `State.Dying` / `State.Dead` | 피격 어빌리티의 Loose 태그 | `TagOnly` | 락온 후보 제외(`TargetableComponent`), 피격 무시, 부활 트리거 |

- `Attack.Window.*`는 서버와 클라이언트가 **각자 자기 몽타주의 노티파이로** 붙인다. 복제하면 §6의 "덮어쓰기"가 생긴다.
- `State.Dying`/`State.Dead`는 **다른 클라이언트의 락온**도 이 대상이 쓰러졌는지 알아야 해서 복제한다. 개수는 필요 없어 `TagOnly`다.

**계층 매칭으로 취소·차단** — [`UBlademasterGameplayAbility_Hit`](../../Source/Blademaster/Combat/BlademasterGameplayAbility_Hit.cpp)

- 콤보 어빌리티의 `AssetTags`는 `Ability.Action.Attack`, 피격 어빌리티는 `Ability.Reaction.Hit`.
- 피격 어빌리티는 부모 `Ability.Action`으로 `CancelAbilities`와 `BlockAbilitiesWithTags`를 부른다.
  M2의 회피·막기도 `Ability.Action.*`만 붙이면 피격에 끊기고 막힌다. §8의 "넓게 막고 좁게 붙인다".
- 차단을 ASC 함수로 직접 걸었기 때문에 `bBlockedActionAbilities`로 해제 짝을 맞춘다(§8).

**이름표로만 쓰는 태그**

- `InputTag.Attack` — Spec의 `DynamicSpecSourceTags`에 넣어 입력과 Spec을 연결하는 키
- `GameplayEvent.WeaponHit` — 이벤트 종류를 구분하는 키이자 트리거 태그
- `SetByCaller.Damage.*` — GE 스펙에 값을 넣고 꺼내는 키

이 태그들은 ASC에 붙지 않는다. 태그 시스템의 **등록된 이름**과 **계층**만 빌려 쓰는 것이다.

**부모 태그로 나열하기** — [`ABlademasterCharacter::DrawOwnCombatDebugText`](../../Source/Blademaster/Characters/BlademasterCharacter.cpp)

- 소유 태그 중 `Attack.Window`의 하위 태그를 모두 나열한다. 새 구간 태그가 생겨도 코드를 고치지 않는다.
- 부모 태그 `Attack.Window`도 네이티브 태그로 정의해 변수로 쓴다. 직접 붙이지는 않고 하위 구간을 찾는 기준으로만 쓴다.
  (처음에는 `RequestGameplayTag(FName("Attack.Window"))`로 문자열로 찾았는데, §2의 위험 때문에 바꿨다.)

## 11. 자주 하는 실수

1. **`RequestGameplayTag`에 문자열을 흩어 쓴다.** 오타가 런타임에만 드러나고, 이름을 바꿀 때 놓친다.
2. **매칭 방향을 반대로 생각한다.** 부모로 자식을 찾을 수 있다고 가정한다(`{"A"}.HasTag("A.1")`은 `false`).
3. **`Exact`가 필요한 곳에 일반 매칭을 쓰거나 그 반대.** 특히 부모 태그가 붙어 있는지만 보고 싶을 때 자식 때문에 `true`가 나온다.
4. **Loose 태그를 붙이고 모든 경로에서 떼지 않는다.** 취소·중단 경로가 빠지기 쉽다.
5. **같은 태그를 여러 곳에서 Loose로 붙이면서 개수를 생각하지 않는다.** 한 번 떼도 남는다.
6. **복제 옵션 기본값이 `None`이라는 걸 모른다.** 서버에서 붙였는데 클라이언트에서 안 보인다.
7. **같은 태그를 로컬로도 붙이고 복제로도 받는다.** 복제가 로컬 개수를 덮는다.
8. **`HasAll({})`가 `true`라는 걸 놓친다.** 비어 있는 요구 조건은 항상 통과한다.
9. **`BlockAbilitiesWithTags`를 직접 부르고 해제 짝을 맞추지 않는다.** 어빌리티가 영원히 막힌다.
10. **`RegisterGameplayTagEvent`로 등록하고 해제하지 않는다.** 등록한 객체가 사라진 뒤 호출된다.
11. **값을 태그 개수로 표현하려 한다.** 개수는 "붙은 이유의 수"이지 수치 저장소가 아니다.

## 12. 자가 점검 질문

1. 네이티브 태그와 `RequestGameplayTag`의 차이는? 네이티브 태그 정의를 헤더에 둘 수 없는 이유는?
2. `{"Ability.Action.Attack"}.HasTag("Ability.Action")`과 `{"Ability.Action"}.HasTag("Ability.Action.Attack")`의 결과와 이유는?
3. `A.B`를 두 번, `A.C`를 한 번 붙였다. `GetTagCount(A)`, `GetExplicitTagCount(A)`, `HasMatchingGameplayTag(A)`는?
4. 같은 태그를 두 번 붙이고 한 번 떼면? 이렇게 개수를 세는 이유는?
5. GE 부여 태그, `ActivationOwnedTags`, Loose 태그 중 짝이 어긋날 위험이 가장 큰 것은? 왜인가?
6. `TagOnly`와 `CountToOwner`는 소유 클라이언트와 다른 클라이언트에게 각각 무엇을 보내나?
7. `Minimal` 복제 모드에서 GE가 부여한 태그는 소유 클라이언트에게 어떻게 도착하나? 다른 클라이언트에게는?
8. 클라이언트가 로컬로 붙인 태그와 같은 이름의 태그가 서버에서 복제되면 어떻게 되나?
9. `ActivationBlockedTags`와 `BlockAbilitiesWithTag`는 각각 무엇을 검사하나?
10. 피격 어빌리티가 `Ability.Action`으로 차단하면 `Ability.Action.Dodge` 어빌리티도 막히는 이유는? 차단 목록에 `Ability.Action.Attack`을 넣었다면 `Ability.Action`만 가진 어빌리티는 막히나?
11. `Attack.Window.*`는 복제하지 않고 `State.Dead`는 복제하는 이유는?
12. "지금 몇 타째인가"를 태그로 표현하지 않는 이유는? 죽은 대상을 무기 판정에서 뺄 때 태그 검사 대신 충돌 채널을 쓰는 게 나은 이유는?

## 13. 엔진 소스 참고 (UE 5.8)

경로 앞부분 `Engine/Plugins/Runtime/GameplayAbilities/Source/GameplayAbilities/`는 `GA/`로,
`Engine/Source/Runtime/GameplayTags/`는 `GT/`로 줄인다.

| 파일 | 위치 | 내용 |
|---|---|---|
| `GT/Classes/GameplayTagContainer.h` | 41-73 | `FGameplayTag`, `RequestGameplayTag`, 이름 비교 |
| 〃 | 78-107 | `MatchesTag` / `MatchesTagExact`와 방향 예시 주석 |
| 〃 | 299-420 | `HasTag` / `HasTagExact` / `HasAny` / `HasAll`와 빈 인자 규칙 |
| 〃 | 737 | `FGameplayTagQuery` |
| `GT/Public/NativeGameplayTags.h` | 31-41 | `UE_DECLARE_GAMEPLAY_TAG_EXTERN`, `UE_DEFINE_GAMEPLAY_TAG(_COMMENT)` |
| `GT/Private/GameplayTagsManager.cpp` | 2372 | `RequestGameplayTag` — 리디렉트, 없으면 `ensure` |
| `GA/Public/GameplayEffectTypes.h` | 1032-1042 | `EGameplayTagEventType` |
| 〃 | 1050-1057 | `EGameplayTagReplicationState` 다섯 가지 |
| 〃 | 1060-1095 | `FGameplayTagCountItem` (태그, 개수, 복제 옵션) |
| 〃 | 1102-1400 | `FGameplayTagCountContainer` — 명시적 개수와 계층 개수 |
| `GA/Private/GameplayEffectTypes.cpp` | 528-676 | 복제 보내기 — 옵션별로 누구에게 무엇을 |
| 〃 | 679-735 | 복제 받기 — 개수 맞추기, 제거 시 전부 제거 |
| 〃 | 785-845 | `UpdateExplicitTags` — 옵션 승격 경고, `TagOnly` 클라이언트 1 제한, 없는 태그 제거 |
| 〃 | 848-870 | 부모 태그까지 개수 전파, 이벤트 호출 조건 |
| `GA/Public/AbilitySystemComponent.h` | 581-620 | `HasMatchingGameplayTag`, `GetOwnedGameplayTags`, `GetTagCount` |
| 〃 | 627-679 | `UpdateTagMap`, `Add/RemoveLooseGameplayTag` (기본 옵션 `None`) |
| 〃 | 720 | `RegisterGameplayTagEvent` |
| 〃 | 1865-1876 | `BlockedAbilityTags`(복제 안 함), `GameplayTagCountContainer`(복제) |
| 〃 | 1921-1943 | 5.7에서 deprecated 된 `MinimalReplicationTags`, `ReplicatedLooseTags` |
| `GA/Private/AbilitySystemComponent.cpp` | 1842-1862 | ASC 복제 프로퍼티 목록 |
| `GA/Private/Abilities/GameplayAbility.cpp` | 349-440 | `DoesAbilitySatisfyTagRequirements` |
| 〃 | 870, 990 | `ActivationOwnedTags`를 `CountToOwner`로 붙이고 뗀다 |
| `GA/Private/AbilitySystemComponent_Abilities.cpp` | 1329-1347 | `CancelAbilities` — `AssetTags.HasAny` |
| 〃 | 1431-1462 | `ApplyAbilityBlockAndCancelTags`, `AreAbilityTagsBlocked`, `Block/UnBlockAbilitiesWithTags` |
| `GA/Private/GameplayEffect.cpp` | 3733-3736 | `ShouldUseMinimalReplication` (`Minimal`·`Mixed`) |
| 〃 | 4658-4679, 4971-4986 | GE 부여 태그의 복제 옵션, 추가·제거 |
| `GA/Public/GameplayAbilitiesDeveloperSettings.h` | 76 | `ReplicateActivationOwnedTags = true` |
| `GA/Private/AbilitySystemBlueprintLibrary.cpp` | 1352, 1364 | 블루프린트 Loose 태그 노드의 복제 옵션 |
