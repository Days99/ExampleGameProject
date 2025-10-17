// Copyright Epic Games, Inc. All Rights Reserved.

#include "ExampleProjectGameMode.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "MyPlayerState.h"
#include "TimerManager.h"
#include "ExampleProjectPlayerController.h"

AExampleProjectGameMode::AExampleProjectGameMode()
{
	// Set default respawn delay
	RespawnDelay = 3.0f;
}

void AExampleProjectGameMode::RespawnPlayer(AController* Controller)
{
	if (!Controller || !HasAuthority())
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("RespawnPlayer called for: %s"), *Controller->GetName());


	// Destroy old pawn if it exists
	APawn* OldPawn = Controller->GetPawn();
	if (OldPawn)
	{
		Controller->UnPossess();
		OldPawn->Destroy();
	}

	// Clear any existing respawn timer for this controller
	if (FTimerHandle* ExistingTimer = PendingRespawns.Find(Controller))
	{
		GetWorldTimerManager().ClearTimer(*ExistingTimer);
	}

	// Start timer for delayed respawn
	FTimerHandle RespawnTimer;
	FTimerDelegate RespawnDelegate;
	RespawnDelegate.BindUFunction(this, FName("RespawnPlayer_Internal"), Controller);
	
	GetWorldTimerManager().SetTimer(
		RespawnTimer,
		RespawnDelegate,
		RespawnDelay,
		false
	);	

	// Store the timer handle
	PendingRespawns.Add(Controller, RespawnTimer);
}

void AExampleProjectGameMode::RespawnPlayer_Internal(AController* Controller)
{
	if (!Controller)
	{
		UE_LOG(LogTemp, Warning, TEXT("RespawnPlayer_Internal: Controller is null"));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("Respawning player: %s"), *Controller->GetName());

	// Reset player's coins if they have a PlayerState
	if (AMyPlayerState* PS = Controller->GetPlayerState<AMyPlayerState>())
	{
		//Reset coins if you want
	}

	// Spawn new pawn at a random player start
	RestartPlayer(Controller);

	// Remove from pending respawns
	PendingRespawns.Remove(Controller);

	UE_LOG(LogTemp, Warning, TEXT("Player %s respawned successfully"), *Controller->GetName());
}
