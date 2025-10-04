// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/StaticMeshComponent.h"
#include "InputActionValue.h"
#include "SimplePawn.generated.h"

class UInputMappingContext;
class UInputAction;

/**
 * Simple Pawn class representing a drone
 * Demonstrates Pawn features: input handling, possession, and basic movement
 */
UCLASS()
class MYPROJECT_API ASimplePawn : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	ASimplePawn();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// Mesh component for the drone body
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* DroneMesh;

	// Spring arm for camera
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USpringArmComponent* SpringArm;

	// Camera component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCameraComponent* Camera;

	// Input Mapping Context
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputMappingContext* DefaultMappingContext;

	// Input Actions
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* LookAction;

	// Movement properties
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float MoveSpeed = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float TurnSpeed = 100.0f;

	// Hover properties
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float HoverHeight = 200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float BobAmplitude = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float BobFrequency = 1.0f;

	// Current hover offset
	float CurrentHoverOffset = 0.0f;

	// Slow turn mode
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	bool bSlowTurnMode = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float SlowTurnSpeed = 50.0f;

	// Target rotation for smooth turning
	FRotator TargetRotation;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Input functions
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);

	// Movement functions
	UFUNCTION(BlueprintCallable, Category = "Movement")
	void SetSlowTurnMode(bool bEnabled);

	// Add movement input with custom implementation
	UFUNCTION(BlueprintCallable, Category = "Movement")
	void AddMovementInputCustom(FVector Direction, float ScaleValue = 1.0f);

	UFUNCTION(BlueprintCallable, Category = "Movement")
	void AddControllerInput(FRotator Rotation, float ScaleValue = 1.0f);
};
