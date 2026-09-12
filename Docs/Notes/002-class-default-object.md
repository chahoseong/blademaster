# 002 — CDO (Class Default Object)

- 기준 엔진: UE 5.8 (엔진 소스 줄 번호는 이 버전 기준이며, 버전이 바뀌면 달라질 수 있다)

---

## 1. CDO란

- 언리얼은 `UObject`를 상속한 클래스마다 **그 클래스의 인스턴스를 하나 미리 만들어 둔다.** 이것이 CDO다.
- 엔진·모듈이 클래스를 로드할 때 **생성자를 실행해서** 만든다.
  [001 — 액터 생애주기와 초기화 순서](001-actor-lifecycle-and-initialization.md)에서 "생성자는 게임 중에만 불리지 않는다"고 한 이유다.
- 이름은 `Default__클래스명` 형식이다. 예: `Default__BP_PlayerCharacter_C`.
- 코드에서는 `GetDefault<T>()` 또는 `SomeClass->GetDefaultObject()`로 접근한다.

> CDO는 **"이 클래스의 기본값이 채워진 견본 인스턴스"**다.
> 게임에서 실제로 쓰는 객체가 아니라, 다른 객체가 참고하는 기준이다.

## 2. 인스턴스가 만들어지는 순서

```
새 인스턴스 생성
 → C++ 생성자 실행              ← 이 시점의 값은 C++ 기본값뿐
 → 견본(CDO 또는 아키타입)의 프로퍼티 값을 복사   ← BP의 Class Defaults 값이 여기서 들어온다
 → (레벨에서 로드한 경우) 저장된 차이 값 적용     ← 레벨에서 개별 수정한 값
```

- Blueprint의 **Class Defaults** 패널에서 설정한 값은 그 BP 클래스의 CDO에 저장되어 있다.
- 그래서 생성자 안에서 BP로 지정하는 프로퍼티(예: `InitializeAttributesEffect`)를 읽으면 아직 null이다.

## 3. 언리얼이 CDO를 쓰는 이유

### ① 새 객체를 만드는 틀
[인스턴스가 만들어지는 순서](#2-인스턴스가-만들어지는-순서)에서 본 것처럼, 모든 인스턴스는 CDO의 값을 복사해서 시작한다.

### ② 저장 파일에는 "차이"만 기록한다
- 레벨에 배치한 액터나 BP 에셋을 저장할 때 모든 프로퍼티를 쓰지 않고 **기본값(CDO)과 다른 값만** 저장한다.
- 파일이 작아진다.
- C++에서 기본값을 바꾸면, 그 값을 따로 바꾸지 않은 인스턴스 전부에 자동으로 반영된다.
- Details 패널의 "기본값으로 되돌리기" 화살표 = "CDO와 다르다"는 표시.

### ③ 네트워크 전송량 절약
리플리케이션도 기본값을 기준으로 비교한다. 처음 전송할 때 기본값과 같은 프로퍼티는 보내지 않는다.

### ④ Blueprint 상속의 토대
BP 클래스(`..._C`)도 자기 CDO를 갖고, 부모 클래스 CDO의 값을 이어받는다.

```
ABlademasterCharacter CDO
 └ ABlademasterPlayerCharacter CDO   ← C++ 생성자가 정한 값 (카메라 거리 400 등)
    └ BP_PlayerCharacter_C CDO       ← BP에서 덮어쓴 값 (메시, AnimClass 등)
```

### ⑤ 인스턴스 없이 클래스 정보 읽기
- "이 클래스의 기본 설정값은?"을 알려고 객체를 만들 필요가 없다. CDO를 읽으면 된다.
- 설정 클래스(ini 값)도 CDO에 로드된다. `GetDefault<UMySettings>()`가 이 방식이다.

## 4. 프로젝트와 GAS에서의 사례

### GameplayEffect는 인스턴스 없이 CDO를 쓴다
- `InitializeAttributesEffect`는 `TSubclassOf<UGameplayEffect>` — 객체가 아니라 **클래스**다.
- `MakeOutgoingSpec`에 클래스를 넘기면 GAS는 내부에서
  `GameplayEffectClass->GetDefaultObject<UGameplayEffect>()`로 CDO를 꺼내 쓴다
  (`AbilitySystemComponent.cpp:535`).
- GE는 "무엇을 할지"만 정의하는 읽기 전용 데이터라 CDO 하나를 모두가 공유한다.
  실행마다 달라지는 값(레벨, 시전자, 컨텍스트)은 따로 만들어지는 **Spec**에 담긴다.
- 정리: **GE 에셋 = 클래스(CDO), 실제 적용 = Spec.** M1에서 공격 GE를 만들 때 계속 쓰인다.

### GameplayAbility의 인스턴싱 정책
- `NonInstanced` 어빌리티는 **CDO 위에서 직접 실행**됐다. 상태를 가질 수 없고 실수하기 쉬워
  UE 5.5부터 deprecated (`GameplayAbilityTypes.h:46`).
- 공격 어빌리티는 기본값인 `InstancedPerActor`를 쓴다.

### CDO의 컴포넌트 서브오브젝트
- 생성자의 `CreateDefaultSubobject`로 만든 컴포넌트는 CDO에도 똑같이 존재하고,
  인스턴스 컴포넌트의 견본이 된다.
- 그래서 BP가 상속받은 컴포넌트 값을 바꾸려면 CDO가 아니라
  **CDO의 컴포넌트 서브오브젝트**(`...Default__BP_PlayerCharacter_C:CharacterMesh0`)를 수정해야 한다.
  CDO 자체에는 그 프로퍼티가 없다.

## 5. 주의사항

1. **생성자에 게임플레이 로직을 넣지 않는다.**
   CDO를 만들 때는 월드도, 다른 액터도, 네트워크도 없다.
   생성자에서 크래시가 나면 **에디터 자체가 켜지지 않는다.**
2. **실행 중에 CDO를 수정하지 않는다.**
   이후에 만들어지는 모든 인스턴스가 영향을 받는다. `GetDefault<T>()`가 `const`를 반환하는 이유다.
3. **C++ 기본값을 바꿔도 반영되지 않는 경우가 있다.**
   BP나 레벨 인스턴스에서 그 값을 이미 덮어써 저장했다면 그 차이 값이 우선한다.
   "C++에서 바꿨는데 그대로"라면 BP에서 덮어쓴 값부터 확인한다.
4. **`ConstructorHelpers::FObjectFinder`는 CDO 생성 시점에 에셋을 강제로 로드한다.**
   에셋이 없거나 경로가 바뀌면 시작 단계부터 문제가 된다.
   생성자에서 에셋 경로를 하드코딩하지 않고 BP에서 지정하기로 한 이유 중 하나다.

## 6. 요약 — 값이 어디에 속하는가

| 위치 | 성격 | 예 |
|---|---|---|
| C++ 생성자 | 모든 인스턴스와 CDO가 공유하는 **틀** | 컴포넌트 생성, 카메라 거리 기본값 |
| BP Class Defaults (CDO) | 틀 위에 덮어쓴 **기본값** | 메시, AnimClass, `InitializeAttributesEffect` |
| BeginPlay 이후 | 개별 인스턴스의 **게임 상태** | 현재 체력, 락온 대상 |

"이 코드는 모든 인스턴스에 똑같이 적용되는 기본값인가, 이 인스턴스만의 상태인가?"를 구분하면
생성자에 둘지 BeginPlay 이후에 둘지가 대부분 정해진다.
