#include "Characters/BlademasterPlayerCharacter.h"

#include "AbilitySystemComponent.h"
#include "BlademasterGameplayTags.h"
#include "Camera/CameraComponent.h"
#include "Combat/BlademasterCombatComponent.h"
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

	const bool bIsAttacking = AbilitySystemComponent && AbilitySystemComponent->HasMatchingGameplayTag(BlademasterGameplayTags::State_Attacking);
	if (!bIsAttacking)
	{
		// 공격 중이 아니므로, 다음 공격 때 스틱 방향을 새로 잡도록 캐시를 비워둔다.
		bFreeAimDirectionCached = false;
	}

	const AActor* Target = TargetingComponent ? TargetingComponent->GetCurrentTarget() : nullptr;
	if (!Target)
	{
		return;
	}

	// 몸통 회전은 공격 중엔 Motion Warping이 전담해야 하므로, 컨트롤러 요 추종을 잠시 끈다.
	// 카메라(ControlRotation 자체)는 아래에서 공격 중에도 계속 갱신한다 — 그래야 공격이 끝났을 때
	// 시점이 그동안 밀린 각도를 한 번에 따라잡느라 튀는 일이 없다.
	bUseControllerRotationYaw = !bIsAttacking;

	if (AController* PlayerController = GetController())
	{
		const FRotator DesiredRotation = (Target->GetActorLocation() - GetActorLocation()).Rotation();
		const FRotator NewRotation = FMath::RInterpTo(PlayerController->GetControlRotation(), DesiredRotation, DeltaSeconds, LockOnRotationInterpSpeed);
		PlayerController->SetControlRotation(NewRotation);
	}
}

FRotator ABlademasterPlayerCharacter::GetAttackDirection() const
{
	// 락온 중에는 이 적을 계속 상대한다는 락온의 의도대로, 콤보 내내 매번 실시간으로 대상을 향한다.
	if (TargetingComponent && TargetingComponent->IsLockedOn())
	{
		if (const AActor* Target = TargetingComponent->GetCurrentTarget())
		{
			const FVector ToTarget = Target->GetActorLocation() - GetActorLocation();
			return FRotator(0.f, ToTarget.Rotation().Yaw, 0.f);
		}
	}

	// 락온이 아닐 때는 콤보 전체에 걸쳐 스틱 방향을 한 번만 반영한다 — 매 타 재조준하면
	// 자유롭게 빙빙 도는 느낌이 나서, 공격을 시작한 방향에 "커밋"하는 쪽이 자연스럽다.
	if (!bFreeAimDirectionCached)
	{
		const FVector LastInput = GetCharacterMovement()->GetLastInputVector();
		CachedFreeAimDirection = LastInput.IsNearlyZero() ? GetActorRotation() : FRotator(0.f, LastInput.Rotation().Yaw, 0.f);
		bFreeAimDirectionCached = true;
	}

	return CachedFreeAimDirection;
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
		EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Started, this, &ABlademasterPlayerCharacter::AttackPressed);
		EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Completed, this, &ABlademasterPlayerCharacter::AttackReleased);
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

void ABlademasterPlayerCharacter::AttackPressed(const FInputActionValue& Value)
{
	if (UBlademasterCombatComponent* Combat = GetCombatComponent())
	{
		Combat->AbilityInputTagPressed(BlademasterGameplayTags::InputTag_Attack);
	}
}

void ABlademasterPlayerCharacter::AttackReleased(const FInputActionValue& Value)
{
	if (UBlademasterCombatComponent* Combat = GetCombatComponent())
	{
		Combat->AbilityInputTagReleased(BlademasterGameplayTags::InputTag_Attack);
	}
}
