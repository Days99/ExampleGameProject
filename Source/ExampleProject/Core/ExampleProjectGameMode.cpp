// Copyright Epic Games, Inc. All Rights Reserved.

#include "ExampleProjectGameMode.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "MyPlayerState.h"
#include "TimerManager.h"
#include "ExampleProjectPlayerController.h"
#include "EngineUtils.h"
#include "../Gameplay/PhysicsReplicatedActor.h"
#include "Server/VoiceRoomManager.h"

AExampleProjectGameMode::AExampleProjectGameMode()
{
	// Set default respawn delay
	RespawnDelay = 3.0f;
	VoiceRoomManager = nullptr;
}

void AExampleProjectGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
	
	// Initialize voice manager on server
	if (HasAuthority())
	{
		InitializeVoiceManager();
	}
}

void AExampleProjectGameMode::InitializeVoiceManager()
{
	if (!HasAuthority() || VoiceRoomManager)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[GameMode] Initializing Voice Room Manager..."));

	// Spawn voice room manager
	VoiceRoomManager = GetWorld()->SpawnActor<AVoiceRoomManager>();
	if (VoiceRoomManager)
	{
		// Bind to credentials ready event
		VoiceRoomManager->OnVoiceCredentialsReady.AddDynamic(this, &AExampleProjectGameMode::OnVoiceCredentialsReady);
		UE_LOG(LogTemp, Log, TEXT("[GameMode] Voice Room Manager initialized"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[GameMode] Failed to spawn Voice Room Manager"));
	}
}

void AExampleProjectGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (!NewPlayer)
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("========================================"));
	UE_LOG(LogTemp, Warning, TEXT("PostLogin called for PlayerController: %s"), *NewPlayer->GetName());
	
	// Find all PhysicsReplicatedActor instances in the world
	int32 ActorCount = 0;
	for (TActorIterator<APhysicsReplicatedActor> It(GetWorld()); It; ++It)
	{
		APhysicsReplicatedActor* PhysicsActor = *It;
		if (PhysicsActor)
		{
			// Set ownership to this player controller
			PhysicsActor->SetOwnershipToPlayer(NewPlayer);
			ActorCount++;
			
			UE_LOG(LogTemp, Warning, TEXT("Set ownership of %s to %s"), 
				*PhysicsActor->GetName(), *NewPlayer->GetName());
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("Total PhysicsReplicatedActors owned by %s: %d"), 
		*NewPlayer->GetName(), ActorCount);
	UE_LOG(LogTemp, Warning, TEXT("========================================"));

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

void AExampleProjectGameMode::RequestVoiceCredentialsForPlayer(APlayerController* PlayerController, const FString& ProductUserId)
{
	if (!HasAuthority() || !VoiceRoomManager || !PlayerController)
	{
		UE_LOG(LogTemp, Error, TEXT("[GameMode] Cannot request voice credentials - invalid state"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[GameMode] Requesting voice credentials for player %s (ProductUserId: %s)"), 
		*PlayerController->GetName(), *ProductUserId);

	// Store player controller for when credentials are ready
	PendingVoiceRequests.Add(ProductUserId, PlayerController);

	// Request credentials from voice room manager
	VoiceRoomManager->AutoAssignMainChannel(ProductUserId, MainVoiceChannelName);
}

void AExampleProjectGameMode::OnVoiceCredentialsReady(const FString& ProductUserId, FVoiceRoomCredentials Credentials)
{
	UE_LOG(LogTemp, Log, TEXT("[GameMode] Voice credentials ready for ProductUserId: %s"), *ProductUserId);

	// Find the player controller that requested these credentials
	APlayerController** FoundPC = PendingVoiceRequests.Find(ProductUserId);
	if (!FoundPC || !*FoundPC)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameMode] No pending request found for ProductUserId: %s"), *ProductUserId);
		return;
	}

	APlayerController* PlayerController = *FoundPC;
	PendingVoiceRequests.Remove(ProductUserId);

	if (AExampleProjectPlayerController* PC = Cast<AExampleProjectPlayerController>(PlayerController))
	{
		if (Credentials.bIsValid)
		{
			// Send credentials to client
			UE_LOG(LogTemp, Log, TEXT("[GameMode] Sending voice credentials to client: Channel=%s, Token=%s (len=%d)"),
				*Credentials.RoomName,
				Credentials.ParticipantToken.IsEmpty() ? TEXT("None") : TEXT("Provided"),
				Credentials.ParticipantToken.Len());
			PC->Client_ReceiveVoiceCredentials(Credentials.RoomName, Credentials.ParticipantToken);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[GameMode] Invalid credentials received for ProductUserId: %s"), *ProductUserId);
		}
	}
}
