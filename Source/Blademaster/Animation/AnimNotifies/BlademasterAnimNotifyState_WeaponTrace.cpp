#include "Animation/AnimNotifies/BlademasterAnimNotifyState_WeaponTrace.h"

#include "Characters/BlademasterCharacter.h"
#include "Combat/BlademasterWeaponTraceComponent.h"
#include "Components/SkeletalMeshComponent.h"

namespace
{
	UBlademasterWeaponTraceComponent* GetAuthoritativeWeaponTraceComponent(const USkeletalMeshComponent* MeshComp)
	{
		ABlademasterCharacter* Character = MeshComp ? Cast<ABlademasterCharacter>(MeshComp->GetOwner()) : nullptr;
		if (!Character || !Character->HasAuthority())
		{
			return nullptr;
		}

		return Character->GetWeaponTraceComponent();
	}
}

void UBlademasterAnimNotifyState_WeaponTrace::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	if (UBlademasterWeaponTraceComponent* Component = GetAuthoritativeWeaponTraceComponent(MeshComp))
	{
		Component->BeginTrace();
	}
}

void UBlademasterAnimNotifyState_WeaponTrace::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	if (UBlademasterWeaponTraceComponent* Component = GetAuthoritativeWeaponTraceComponent(MeshComp))
	{
		Component->TickTrace();
	}
}

void UBlademasterAnimNotifyState_WeaponTrace::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	if (UBlademasterWeaponTraceComponent* Component = GetAuthoritativeWeaponTraceComponent(MeshComp))
	{
		Component->EndTrace();
	}
}
