// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputActionValue.h"
#include "CollectibleInterface.h"
#include "HeroCharacter.generated.h"

class UInputMappingContext;
class UInputAction;

/**
 * Enhanced Character class with interaction and movement customization
 * Demonstrates Character features: input, movement, and interaction systems
 */
UCLASS()
class MYPROJECT_API AHeroCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AHeroCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// Camera components (optional - for first person view)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	USpringArmComponent* SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	UCameraComponent* Camera;

	// Input Mapping Context
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputMappingContext* DefaultMappingContext;

	// Input Actions
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* JumpAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* InteractAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* SprintAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* CrouchAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* DashAction;

	// Movement customization properties
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float SprintSpeedMultiplier = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float DashDistance = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float DashCooldown = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float AirControlMultiplier = 2.0f;

	// Interaction properties
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	float InteractionRange = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	TEnumAsByte<ECollisionChannel> InteractionChannel = ECC_Visibility;

	// State variables
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bIsSprinting = false;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bCanDash = true;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bIsCrouching = false;

	// Original movement values
	float OriginalMaxWalkSpeed;
	float OriginalAirControl;
	float OriginalCapsuleHalfHeight;

	// Timers
	FTimerHandle DashCooldownTimer;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Input functions
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void Jump() override;
	void StopJumping() override;
	void Interact();
	void StartSprint();
	void StopSprint();
	void StartCrouch();
	void StopCrouch();
	void Dash();

	// Movement functions
	UFUNCTION(BlueprintCallable, Category = "Movement")
	void SetSprintSpeed(bool bSprinting);

	UFUNCTION(BlueprintCallable, Category = "Movement")
	void SetCrouchHeight(bool bCrouching);

	UFUNCTION(BlueprintCallable, Category = "Movement")
	void ApplyAirControlMultiplier();

	UFUNCTION(BlueprintCallable, Category = "Movement")
	void RemoveAirControlMultiplier();

	// Interaction functions
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	AActor* GetActorInSight();

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	bool TryInteractWithActor(AActor* TargetActor);

	// Timer functions
	UFUNCTION()
	void ResetDashCooldown();

	// Event dispatchers
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteract, AActor*, InteractedActor);
	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FOnInteract OnInteract;
};
