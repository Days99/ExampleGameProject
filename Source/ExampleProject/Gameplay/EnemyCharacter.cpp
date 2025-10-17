// Fill out your copyright notice in the Description page of Project Settings.


#include "EnemyCharacter.h"
#include <Kismet/GameplayStatics.h>
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include <Core/ExampleProjectCharacter.h>
#include <Core/CoinsGameStateBase.h>
#include <Core/ExampleProjectGameMode.h>
#include "Net/UnrealNetwork.h"
#include "EnemyAIController.h"

// Sets default values
AEnemyCharacter::AEnemyCharacter()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Enable replication
	bReplicates = true;
	SetReplicateMovement(true);

	// Initialize replicated properties
	CurrentSpeed = 100.0f;
	AnimationRate = 1.0f;

	// Configure character movement for proper animation
	GetCharacterMovement()->bUseControllerDesiredRotation = true;
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // Smooth rotation
	GetCharacterMovement()->MaxWalkSpeed = 100.0f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.0f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.0f;
	
	// Set the AI controller class for this enemy
	AIControllerClass = AEnemyAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

void AEnemyCharacter::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!HasAuthority()) return;
	
	// Only respond to player characters, not other enemies or random actors
	AExampleProjectCharacter* PlayerCharacter = Cast<AExampleProjectCharacter>(Actor);
	if (!PlayerCharacter)
	{
		// Not a player character, ignore this stimulus
		return;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("Enemy %s: Perception updated for PLAYER %s, Successfully sensed: %d"), 
		*GetName(), *Actor->GetName(), Stimulus.WasSuccessfullySensed());
	
	// Check if the stimulus was successfully sensed (seen)
	if (Stimulus.WasSuccessfullySensed())
	{
		// Player detected - start chasing if we don't have a target or if this is our current target
		if (AIController && (!target || target == PlayerCharacter))
		{
			target = PlayerCharacter;
			SetEnemySpeed(150.0f, 2.5f);
			AIController->MoveToActor(PlayerCharacter);
			
			UE_LOG(LogTemp, Warning, TEXT("Enemy %s detected player: %s - CHASING!"), *GetName(), *Actor->GetName());
		}
	}
	else
	{
		// Actor was lost from sight - only react if this was our target
		if (Actor == target)
		{
			SetEnemySpeed(100.0f, 1.0f);
			target = nullptr;
			
			if (AIController && Waypoints.Num() > 0)
			{
				AIController->MoveToActor(GetRandomWaypoint());
			}

			UE_LOG(LogTemp, Warning, TEXT("Enemy %s lost target, returning to patrol"), *GetName());
		}
	}
}




// Called when the game starts or when spawned
void AEnemyCharacter::BeginPlay()  {
	 Super::BeginPlay();  
	 UGameplayStatics::GetAllActorsOfClass(GetWorld(),  ATargetPoint::StaticClass(), Waypoints);  
	 
	 AIController = Cast<AAIController>(GetController()); 
	if ((Waypoints.Num() > 0) && (AIController))
	{  
		AIController->GetPathFollowingComponent()->OnRequestFinished.AddUObject(this,  &AEnemyCharacter::AIMoveCompleted);  
		
		if (HasAuthority()) 
		{
			AIController->MoveToActor(GetRandomWaypoint());  
		}
	}
	
	UE_LOG(LogTemp, Warning, TEXT("Enemy %s: BeginPlay complete, controller: %s"), 
		*GetName(), AIController ? *AIController->GetName() : TEXT("NULL"));
}



ATargetPoint* AEnemyCharacter::GetRandomWaypoint()  
{  
	int index = FMath::RandRange(0, Waypoints.Num() - 1);  
	return Cast<ATargetPoint>(Waypoints[index]);  
}

void AEnemyCharacter::AIMoveCompleted(FAIRequestID RequestID,  const FPathFollowingResult& Result)
{  
	if (!HasAuthority()) return;
	
	if (Result.IsSuccess())  
	{
		if (target) 
		{ 
			if (target->GetClass()->ImplementsInterface(UDamageable::StaticClass()))
				IDamageable::Execute_TakeDamage(target, 1, this);
			// Clear target and return to patrol
			target = nullptr;
			SetEnemySpeed(100.0f, 1.0f);
		}
		
		// Continue patrolling
		if ((Waypoints.Num() > 0) && (AIController))  
		{
			AIController->MoveToActor(GetRandomWaypoint());  
		}
	}

}

void AEnemyCharacter::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);  


}

// Called to bind functionality to input
void AEnemyCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

// Replication functions
void AEnemyCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AEnemyCharacter, CurrentSpeed);
	DOREPLIFETIME(AEnemyCharacter, AnimationRate);
}

void AEnemyCharacter::SetEnemySpeed(float NewSpeed, float NewAnimRate)
{
	if (HasAuthority())
	{
		CurrentSpeed = NewSpeed;
		AnimationRate = NewAnimRate;
		
		// Update locally on server
		OnRep_CurrentSpeed();
		OnRep_AnimationRate();
	}
}

void AEnemyCharacter::OnRep_CurrentSpeed()
{
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = CurrentSpeed;
		UE_LOG(LogTemp, Log, TEXT("Enemy speed updated to: %f"), CurrentSpeed);
	}
}

void AEnemyCharacter::OnRep_AnimationRate()
{
	if (GetMesh())
	{
		GetMesh()->GlobalAnimRateScale = AnimationRate;
		UE_LOG(LogTemp, Log, TEXT("Enemy animation rate updated to: %f"), AnimationRate);
	}
}

