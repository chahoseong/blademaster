#include "AbilitySystem/BlademasterGameplayEffectContext.h"

#include "BlademasterLogChannels.h"

FBlademasterGameplayEffectContext* FBlademasterGameplayEffectContext::FromHandle(FGameplayEffectContextHandle& Handle)
{
	return const_cast<FBlademasterGameplayEffectContext*>(FromHandle(static_cast<const FGameplayEffectContextHandle&>(Handle)));
}

const FBlademasterGameplayEffectContext* FBlademasterGameplayEffectContext::FromHandle(const FGameplayEffectContextHandle& Handle)
{
	const FGameplayEffectContext* Context = Handle.Get();
	if (!Context)
	{
		return nullptr;
	}

	if (Context->GetScriptStruct() != FBlademasterGameplayEffectContext::StaticStruct())
	{
		UE_LOG(LogBlademasterCombat, Warning, TEXT("GameplayEffectContext가 FBlademasterGameplayEffectContext가 아니다 (%s). AbilitySystemGlobals의 컨텍스트 설정을 확인할 것"),
			*GetNameSafe(Context->GetScriptStruct()));
		return nullptr;
	}

	return static_cast<const FBlademasterGameplayEffectContext*>(Context);
}
