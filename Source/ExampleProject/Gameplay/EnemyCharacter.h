// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AIController.h"
#include "Engine/TargetPoint.h"
#include "Perception/AIPerceptionTypes.h"
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

	AActor* target;

	// Replicated properties for movement and animation
	UPROPERTY(ReplicatedUsing = OnRep_CurrentSpeed)
	float CurrentSpeed;

	UPROPERTY(ReplicatedUsing = OnRep_AnimationRate)
	float AnimationRate;

	UFUNCTION()
	void OnRep_CurrentSpeed();

	UFUNCTION()
	void OnRep_AnimationRate();

	void SetEnemySpeed(float NewSpeed, float NewAnimRate);


public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// Replication
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Called by AIController when perception is updated (public so controller can access it)
	void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

};
