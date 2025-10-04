// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AIController.h"
#include "Engine/TargetPoint.h"
#include "EnemyCharacter.generated.h"


UCLASS()
class EXAMPLEPROJECT_API AEnemyCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AEnemyCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	TArray<AActor*> Waypoints;
	
	AAIController* AIController;
	
	ATargetPoint* GetRandomWaypoint();
	
	void AIMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result);


public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};
