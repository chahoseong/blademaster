#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "BlademasterGameplayEffectContext.generated.h"

// 데미지 GE를 공격자의 ASC로 만들 때, 실제로 얼마나 깎을지(공격 데이터의 값)를 함께 싣고 다닌다.
// 판정(#12)이 GameplayEvent로 보낸 컨텍스트를 피격 어빌리티가 그대로 받아 스펙을 만들 때 쓴다.
USTRUCT()
struct BLADEMASTER_API FBlademasterGameplayEffectContext : public FGameplayEffectContext
{
	GENERATED_BODY()

	UPROPERTY()
	float HealthDamage = 0.f;

	UPROPERTY()
	float PostureDamage = 0.f;

	virtual UScriptStruct* GetScriptStruct() const override
	{
		return FBlademasterGameplayEffectContext::StaticStruct();
	}

	virtual FGameplayEffectContext* Duplicate() const override
	{
		FBlademasterGameplayEffectContext* NewContext = new FBlademasterGameplayEffectContext(*this);
		if (GetHitResult())
		{
			NewContext->AddHitResult(*GetHitResult(), true);
		}
		return NewContext;
	}

	virtual bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess) override;
};

template<>
struct TStructOpsTypeTraits<FBlademasterGameplayEffectContext> : public TStructOpsTypeTraitsBase2<FBlademasterGameplayEffectContext>
{
	enum
	{
		WithNetSerializer = true,
		WithCopy = true
	};
};
