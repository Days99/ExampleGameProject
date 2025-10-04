// Fill out your copyright notice in the Description page of Project Settings.

#include "HeroCharacter.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/KismetSystemLibrary.h"
#include "CollectibleInterface.h"
#include "BasePickup.h"

// Sets default values
AHeroCharacter::AHeroCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 600.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Configure character to use Enhanced Input
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Create spring arm for camera (matching Third Person template)
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = 400.0f; // Third Person template uses 400
	SpringArm->SetRelativeRotation(FRotator(-10.0f, 0.0f, 0.0f)); // Third Person template uses -10
	SpringArm->bDoCollisionTest = true; // Enable collision test like template
	SpringArm->bUsePawnControlRotation = true; // Important: allows controller to rotate spring arm
	SpringArm->bInheritPitch = true;
	SpringArm->bInheritYaw = true;
	SpringArm->bInheritRoll = false;

	// Create camera
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	Camera->bUsePawnControlRotation = false; // Camera inherits rotation from spring arm

	// Store original values
	OriginalMaxWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;
	OriginalAirControl = GetCharacterMovement()->AirControl;
	OriginalCapsuleHalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
}

// Called when the game starts or when spawned
void AHeroCharacter::BeginPlay()
{
	Super::BeginPlay();


	// Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			if (DefaultMappingContext)
			{
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
				
				if (GEngine)
				{
					GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, 
						TEXT("HeroCharacter: Input Mapping Context added successfully"));
				}
			}
			else
			{
				if (GEngine)
				{
					GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, 
						TEXT("HeroCharacter: DefaultMappingContext is NULL!"));
				}
			}
		}
		else
		{
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, 
					TEXT("HeroCharacter: Could not get Enhanced Input Subsystem"));
			}
		}
	}
	else
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, 
				TEXT("HeroCharacter: Controller is not a PlayerController"));
		}
	}
}

// Called to bind functionality to input
void AHeroCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Jumping
		if (JumpAction)
		{
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &AHeroCharacter::Jump);
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &AHeroCharacter::StopJumping);
		}

		// Moving
		if (MoveAction)
		{
			EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AHeroCharacter::Move);
		}

		// Looking
		if (LookAction)
		{
			EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AHeroCharacter::Look);
		}

		// Interacting
		EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Triggered, this, &AHeroCharacter::Interact);

		// Sprinting
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &AHeroCharacter::StartSprint);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &AHeroCharacter::StopSprint);

		// Crouching
		EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Started, this, &AHeroCharacter::StartCrouch);
		EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Completed, this, &AHeroCharacter::StopCrouch);

		// Dashing
		EnhancedInputComponent->BindAction(DashAction, ETriggerEvent::Triggered, this, &AHeroCharacter::Dash);

		// Debug output
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, 
				TEXT("HeroCharacter: All input actions bound successfully"));
		}
	}
	else
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, 
				TEXT("HeroCharacter: Failed to cast to EnhancedInputComponent"));
		}
	}
}

// Called every frame
void AHeroCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Apply air control multiplier when falling

}

// Input function for movement
void AHeroCharacter::Move(const FInputActionValue& Value)
{
	// Input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// Find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
		// Get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		// Get right vector
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// Add movement
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);

	}
}

// Input function for looking
void AHeroCharacter::Look(const FInputActionValue& Value)
{
	// Input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// Add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);

	}
}

// Jump function
void AHeroCharacter::Jump()
{
	Super::Jump();
}

// Stop jumping function
void AHeroCharacter::StopJumping()
{
	Super::StopJumping();
}

// Interact function
void AHeroCharacter::Interact()
{
	AActor* ActorInSight = GetActorInSight();
	if (ActorInSight)
	{
		TryInteractWithActor(ActorInSight);
	}
}

// Start sprinting
void AHeroCharacter::StartSprint()
{
	if (!bIsSprinting && !GetCharacterMovement()->IsCrouching())
	{
		SetSprintSpeed(true);
		bIsSprinting = true;
	}
}

// Stop sprinting
void AHeroCharacter::StopSprint()
{
	if (bIsSprinting)
	{
		SetSprintSpeed(false);
		bIsSprinting = false;
	}
}

// Start crouching
void AHeroCharacter::StartCrouch()
{
	if (!bIsCrouching && !bIsSprinting)
	{
		SetCrouchHeight(true);
		bIsCrouching = true;
	}
}

// Stop crouching
void AHeroCharacter::StopCrouch()
{
	if (bIsCrouching)
	{
		SetCrouchHeight(false);
		bIsCrouching = false;
	}
}

// Dash function
void AHeroCharacter::Dash()
{
	if (bCanDash)
	{
		FVector DashDirection = GetActorForwardVector();
		LaunchCharacter(DashDirection * DashDistance, false, false);
		bCanDash = false;
		GetWorldTimerManager().SetTimer(DashCooldownTimer, this, &AHeroCharacter::ResetDashCooldown, DashCooldown, false);
	}
}

// Set sprint speed
void AHeroCharacter::SetSprintSpeed(bool bSprinting)
{
	if (bSprinting)
	{
		GetCharacterMovement()->MaxWalkSpeed = OriginalMaxWalkSpeed * SprintSpeedMultiplier;
	}
	else
	{
		GetCharacterMovement()->MaxWalkSpeed = OriginalMaxWalkSpeed;
	}
}

// Set crouch height
void AHeroCharacter::SetCrouchHeight(bool bCrouching)
{
	if (bCrouching)
	{
		GetCapsuleComponent()->SetCapsuleHalfHeight(OriginalCapsuleHalfHeight * 0.5f);
		GetCharacterMovement()->MaxWalkSpeed = OriginalMaxWalkSpeed * 0.5f;
	}
	else
	{
		GetCapsuleComponent()->SetCapsuleHalfHeight(OriginalCapsuleHalfHeight);
		GetCharacterMovement()->MaxWalkSpeed = OriginalMaxWalkSpeed;
	}
}

// Apply air control multiplier
void AHeroCharacter::ApplyAirControlMultiplier()
{
	GetCharacterMovement()->AirControl = OriginalAirControl * AirControlMultiplier;
}

// Remove air control multiplier
void AHeroCharacter::RemoveAirControlMultiplier()
{
	GetCharacterMovement()->AirControl = OriginalAirControl;
}

// Get actor in sight
AActor* AHeroCharacter::GetActorInSight()
{
	FVector Start = GetActorLocation();
	FVector End = Start + (GetActorForwardVector() * InteractionRange);

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, InteractionChannel, QueryParams))
	{
		return HitResult.GetActor();
	}

	return nullptr;
}

// Try to interact with actor
bool AHeroCharacter::TryInteractWithActor(AActor* TargetActor)
{
	if (!TargetActor) return false;

	// Check if actor implements collectible interface
	if (ICollectibleInterface* Collectible = Cast<ICollectibleInterface>(TargetActor))
	{
		if (Collectible->Execute_CanBeCollected(TargetActor))
		{
			Collectible->Execute_Collect(TargetActor);
			OnInteract.Broadcast(TargetActor);
			return true;
		}
	}

	// Print debug message
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, 
			FString::Printf(TEXT("Interacted with: %s"), *TargetActor->GetName()));
	}

	OnInteract.Broadcast(TargetActor);
	return false;
}

// Reset dash cooldown
void AHeroCharacter::ResetDashCooldown()
{
	bCanDash = true;
}
