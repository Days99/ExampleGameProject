// Fill out your copyright notice in the Description page of Project Settings.

#include "SimplePawn.h"
#include "Components/StaticMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/PawnMovementComponent.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/Engine.h"

// Sets default values
ASimplePawn::ASimplePawn()
{
 	// Set this pawn to call Tick() every frame
	PrimaryActorTick.bCanEverTick = true;

	// Create mesh component
	DroneMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DroneMesh"));
	RootComponent = DroneMesh;

	// Set default mesh (basic cube)
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube"));
	if (CubeMesh.Succeeded())
	{
		DroneMesh->SetStaticMesh(CubeMesh.Object);
		DroneMesh->SetRelativeScale3D(FVector(0.5f, 0.5f, 0.2f));
	}

	// Create spring arm
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = 800.0f;
	SpringArm->SetRelativeRotation(FRotator(-45.0f, 0.0f, 0.0f));
	SpringArm->bDoCollisionTest = false;

	// Create camera
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);

	// Set up movement component
	//GetMovementComponent()->UpdatedComponent = RootComponent;

	// Initialize target rotation
	TargetRotation = GetActorRotation();
}

// Called when the game starts or when spawned
void ASimplePawn::BeginPlay()
{
	Super::BeginPlay();

	// Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

// Called to bind functionality to input
void ASimplePawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ASimplePawn::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ASimplePawn::Look);
	}
}

// Called every frame
void ASimplePawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Update hover bobbing
	CurrentHoverOffset += BobFrequency * DeltaTime;
	float BobOffset = FMath::Sin(CurrentHoverOffset) * BobAmplitude;
	
	// Apply hover height and bobbing
	FVector CurrentLocation = GetActorLocation();
	CurrentLocation.Z = HoverHeight + BobOffset;
	SetActorLocation(CurrentLocation);

	// Handle smooth turning
	if (bSlowTurnMode)
	{
		FRotator CurrentRotation = GetActorRotation();
		FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, SlowTurnSpeed);
		SetActorRotation(NewRotation);
	}
}

// Input function for movement
void ASimplePawn::Move(const FInputActionValue& Value)
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
		AddMovementInputCustom(ForwardDirection, MovementVector.Y);
		AddMovementInputCustom(RightDirection, MovementVector.X);
	}
}

// Input function for looking
void ASimplePawn::Look(const FInputActionValue& Value)
{
	// Input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// Add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);

		// Update target rotation for smooth turning
		if (bSlowTurnMode)
		{
			TargetRotation = Controller->GetControlRotation();
		}
	}
}

// Set slow turn mode
void ASimplePawn::SetSlowTurnMode(bool bEnabled)
{
	bSlowTurnMode = bEnabled;
	if (!bSlowTurnMode)
	{
		TargetRotation = GetActorRotation();
	}
}

// Add movement input with custom implementation
void ASimplePawn::AddMovementInputCustom(FVector Direction, float ScaleValue)
{
	if (GetMovementComponent() && (GetMovementComponent()->UpdatedComponent == RootComponent))
	{
		GetMovementComponent()->AddInputVector(Direction * ScaleValue);
	}
}

// Add controller input
void ASimplePawn::AddControllerInput(FRotator Rotation, float ScaleValue)
{
	if (Controller)
	{
		AddControllerYawInput(Rotation.Yaw * ScaleValue);
		AddControllerPitchInput(Rotation.Pitch * ScaleValue);
	}
}
