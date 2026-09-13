#include "Animation/AnimNotifies/BlademasterAnimNotifyState_GameplayTag.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Components/SkeletalMeshComponent.h"

void UBlademasterAnimNotifyState_GameplayTag::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	const IAbilitySystemInterface* AbilitySystemInterface = MeshComp ? Cast<IAbilitySystemInterface>(MeshComp->GetOwner()) : nullptr;
	if (UAbilitySystemComponent* AbilitySystemComponent = AbilitySystemInterface ? AbilitySystemInterface->GetAbilitySystemComponent() : nullptr)
	{
		AbilitySystemComponent->AddLooseGameplayTag(Tag);
	}
}

void UBlademasterAnimNotifyState_GameplayTag::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	const IAbilitySystemInterface* AbilitySystemInterface = MeshComp ? Cast<IAbilitySystemInterface>(MeshComp->GetOwner()) : nullptr;
	if (UAbilitySystemComponent* AbilitySystemComponent = AbilitySystemInterface ? AbilitySystemInterface->GetAbilitySystemComponent() : nullptr)
	{
		AbilitySystemComponent->RemoveLooseGameplayTag(Tag);
	}
}

FString UBlademasterAnimNotifyState_GameplayTag::GetNotifyName_Implementation() const
{
	return Tag.IsValid() ? Tag.ToString() : Super::GetNotifyName_Implementation();
}
