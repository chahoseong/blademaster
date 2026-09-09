#include "Characters/BlademasterPlayerCharacter.h"

#include "Camera/CameraComponent.h"
#include "Combat/BlademasterTargetingComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputActionValue.h"

ABlademasterPlayerCharacter::ABlademasterPlayerCharacter()
{
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 500.f, 0.f);

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.f;
	CameraBoom->SocketOffset = FVector(0.f, 0.f, 60.f);
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	TargetingComponent = CreateDefaultSubobject<UBlademasterTargetingComponent>(TEXT("TargetingComponent"));

	GetMesh()->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -90.f), FRotator(0.f, -90.f, 0.f));
}

void ABlademasterPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (TargetingComponent)
	{
		TargetingComponent->OnLockOnTargetChanged.AddUObject(this, &ABlademasterPlayerCharacter::OnLockOnTargetChanged);
	}
}

void ABlademasterPlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const AActor* Target = TargetingComponent ? TargetingComponent->GetCurrentTarget() : nullptr;
	if (!Target)
	{
		return;
	}

	if (AController* PlayerController = GetController())
	{
		const FRotator DesiredRotation = (Target->GetActorLocation() - GetActorLocation()).Rotation();
		const FRotator NewRotation = FMath::RInterpTo(PlayerController->GetControlRotation(), DesiredRotation, DeltaSeconds, LockOnRotationInterpSpeed);
		PlayerController->SetControlRotation(NewRotation);
	}
}

void ABlademasterPlayerCharacter::OnLockOnTargetChanged(AActor* NewTarget)
{
	bUseControllerRotationYaw = (NewTarget != nullptr);
	GetCharacterMovement()->bOrientRotationToMovement = (NewTarget == nullptr);
}

void ABlademasterPlayerCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void ABlademasterPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ABlademasterPlayerCharacter::Move);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ABlademasterPlayerCharacter::Look);
		EnhancedInputComponent->BindAction(LockOnAction, ETriggerEvent::Started, this, &ABlademasterPlayerCharacter::LockOn);
		EnhancedInputComponent->BindAction(SwitchTargetMouseAction, ETriggerEvent::Triggered, this, &ABlademasterPlayerCharacter::SwitchTargetMouse);
		EnhancedInputComponent->BindAction(SwitchTargetStickAction, ETriggerEvent::Triggered, this, &ABlademasterPlayerCharacter::SwitchTargetStick);
		EnhancedInputComponent->BindAction(SwitchTargetStickAction, ETriggerEvent::Completed, this, &ABlademasterPlayerCharacter::ResetSwitchTargetStick);
	}
}

void ABlademasterPlayerCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller)
	{
		const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void ABlademasterPlayerCharacter::Look(const FInputActionValue& Value)
{
	if (TargetingComponent && TargetingComponent->IsLockedOn())
	{
		return;
	}

	const FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller)
	{
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void ABlademasterPlayerCharacter::LockOn(const FInputActionValue& Value)
{
	if (TargetingComponent)
	{
		TargetingComponent->ToggleLockOn();
	}
}

void ABlademasterPlayerCharacter::SwitchTargetMouse(const FInputActionValue& Value)
{
	if (TargetingComponent)
	{
		TargetingComponent->EvaluateMouseSwitchInput(Value.Get<float>(), GetWorld()->GetDeltaSeconds());
	}
}

void ABlademasterPlayerCharacter::SwitchTargetStick(const FInputActionValue& Value)
{
	if (TargetingComponent)
	{
		TargetingComponent->EvaluateStickSwitchInput(Value.Get<float>());
	}
}

void ABlademasterPlayerCharacter::ResetSwitchTargetStick(const FInputActionValue& Value)
{
	if (TargetingComponent)
	{
		TargetingComponent->ResetStickSwitchGesture();
	}
}
