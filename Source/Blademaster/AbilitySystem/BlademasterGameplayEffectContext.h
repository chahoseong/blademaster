#pragma once

#include "Combat/BlademasterHitDirection.h"
#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "BlademasterGameplayEffectContext.generated.h"

// 데미지 GE를 공격자의 ASC로 만들 때, 실제로 얼마나 깎을지(공격 데이터의 값)를 함께 싣고 다닌다.
// 판정(#12)이 GameplayEvent로 보낸 컨텍스트를 CombatComponent가 그대로 받아 스펙을 만들 때 쓴다.
USTRUCT()
struct BLADEMASTER_API FBlademasterGameplayEffectContext : public FGameplayEffectContext
{
	GENERATED_BODY()

	UPROPERTY()
	float HealthDamage = 0.f;

	UPROPERTY()
	float PostureDamage = 0.f;

	// CombatComponent가 HitResult를 해석해 채우고, 반응 어빌리티가 몽타주를 고를 때 읽는다.
	UPROPERTY()
	EBlademasterHitDirection HitDirection = EBlademasterHitDirection::Front;

	// 핸들이 가리키는 컨텍스트가 이 타입이면 내려주고, 아니면 경고를 남기고 nullptr을 돌려준다.
	// 타입 불일치는 런타임 조건이 아니라 AbilitySystemGlobals 설정 오류다.
	static FBlademasterGameplayEffectContext* FromHandle(FGameplayEffectContextHandle& Handle);
	static const FBlademasterGameplayEffectContext* FromHandle(const FGameplayEffectContextHandle& Handle);

	virtual UScriptStruct* GetScriptStruct() const override
	{
		return FBlademasterGameplayEffectContext::StaticStruct();
	}

	// 복사 생성자가 모든 필드를 복사한다. HitResult만 별도로 다시 담는다.
	virtual FGameplayEffectContext* Duplicate() const override
	{
		FBlademasterGameplayEffectContext* NewContext = new FBlademasterGameplayEffectContext(*this);
		if (GetHitResult())
		{
			NewContext->AddHitResult(*GetHitResult(), true);
		}
		return NewContext;
	}
};

template<>
struct TStructOpsTypeTraits<FBlademasterGameplayEffectContext> : public TStructOpsTypeTraitsBase2<FBlademasterGameplayEffectContext>
{
	enum
	{
		WithCopy = true
	};
};
