// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ExampleProjectGameMode.generated.h"

class AVoiceRoomManager;

/**
 *  Simple GameMode for a third person game
 */
UCLASS(abstract)
class AExampleProjectGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	
	/** Constructor */
	AExampleProjectGameMode();

	/** Initialize game - set up voice manager */
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;

	/** Called after a successful login - set up ownership for physics actors */
	virtual void PostLogin(APlayerController* NewPlayer) override;

	/** Request voice credentials for a player (called from PlayerController RPC) */
	void RequestVoiceCredentialsForPlayer(APlayerController* PlayerController, const FString& ProductUserId);

	/** Respawn a player after they die */
	UFUNCTION()
	void RespawnPlayer(AController* Controller);

protected:
	/** Time to wait before respawning (seconds) */
	UPROPERTY(EditDefaultsOnly, Category = "Respawn")
	float RespawnDelay = 3.0f;

	/** Map of controllers waiting to respawn and their timer handles */
	TMap<AController*, FTimerHandle> PendingRespawns;

	/** Map of pending voice credential requests (ProductUserId -> PlayerController) */
	TMap<FString, APlayerController*> PendingVoiceRequests;

	/** Internal function called after delay to actually respawn */
	UFUNCTION()
	void RespawnPlayer_Internal(AController* Controller);

	/** Voice room manager instance */
	UPROPERTY()
	AVoiceRoomManager* VoiceRoomManager;

	/** Main voice channel name */
	UPROPERTY(EditDefaultsOnly, Category = "Voice Chat")
	FString MainVoiceChannelName = TEXT("MainChannel");

	/** Initialize voice room manager */
	void InitializeVoiceManager();

	/** Handle voice credentials ready callback */
	UFUNCTION()
	void OnVoiceCredentialsReady(const FString& ProductUserId, FVoiceRoomCredentials Credentials);
};



