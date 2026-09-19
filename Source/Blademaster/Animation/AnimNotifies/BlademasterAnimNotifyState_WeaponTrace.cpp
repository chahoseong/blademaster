#include "Animation/AnimNotifies/BlademasterAnimNotifyState_WeaponTrace.h"

#include "Characters/BlademasterCharacter.h"
#include "Combat/BlademasterWeaponTraceComponent.h"
#include "Components/SkeletalMeshComponent.h"

namespace
{
	UBlademasterWeaponTraceComponent* GetWeaponTraceComponent(const USkeletalMeshComponent* MeshComp)
	{
		ABlademasterCharacter* Character = MeshComp ? Cast<ABlademasterCharacter>(MeshComp->GetOwner()) : nullptr;
		return Character ? Character->GetWeaponTraceComponent() : nullptr;
	}
}

void UBlademasterAnimNotifyState_WeaponTrace::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	if (UBlademasterWeaponTraceComponent* Component = GetWeaponTraceComponent(MeshComp))
	{
		Component->BeginTrace();
	}
}

void UBlademasterAnimNotifyState_WeaponTrace::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	if (UBlademasterWeaponTraceComponent* Component = GetWeaponTraceComponent(MeshComp))
	{
		Component->TickTrace();
	}
}

void UBlademasterAnimNotifyState_WeaponTrace::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	if (UBlademasterWeaponTraceComponent* Component = GetWeaponTraceComponent(MeshComp))
	{
		Component->EndTrace();
	}
}
