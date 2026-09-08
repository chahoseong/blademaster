#include "Animation/BlademasterAnimInstance.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"

void UBlademasterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	OwningCharacter = Cast<ACharacter>(TryGetPawnOwner());
}

void UBlademasterAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!OwningCharacter)
	{
		OwningCharacter = Cast<ACharacter>(TryGetPawnOwner());
	}

	if (!OwningCharacter)
	{
		MoveForward = 0.f;
		MoveRight = 0.f;
		HorizontalAngle = 0.f;
		VerticalAngle = 0.f;
		return;
	}

	const UCharacterMovementComponent* MovementComponent = OwningCharacter->GetCharacterMovement();
	const float MaxSpeed = MovementComponent ? MovementComponent->GetMaxSpeed() : 0.f;

	if (MaxSpeed > KINDA_SMALL_NUMBER)
	{
		const FVector LocalVelocity = OwningCharacter->GetActorTransform().InverseTransformVectorNoScale(OwningCharacter->GetVelocity());
		MoveForward = FMath::Clamp(LocalVelocity.X / MaxSpeed, -1.f, 1.f);
		MoveRight = FMath::Clamp(LocalVelocity.Y / MaxSpeed, -1.f, 1.f);
	}
	else
	{
		MoveForward = 0.f;
		MoveRight = 0.f;
	}

	if (const AController* Controller = OwningCharacter->GetController())
	{
		const FRotator ControlRotation = Controller->GetControlRotation().GetNormalized();
		const float ActorYaw = OwningCharacter->GetActorRotation().Yaw;

		HorizontalAngle = FMath::Clamp(FMath::FindDeltaAngleDegrees(ActorYaw, ControlRotation.Yaw), -90.f, 90.f);
		VerticalAngle = FMath::Clamp(ControlRotation.Pitch, -90.f, 90.f);
	}
	else
	{
		HorizontalAngle = 0.f;
		VerticalAngle = 0.f;
	}
}
