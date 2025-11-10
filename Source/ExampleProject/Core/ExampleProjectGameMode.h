// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ExampleProjectGameMode.generated.h"

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

	/** Called after a successful login - set up ownership for physics actors */
	virtual void PostLogin(APlayerController* NewPlayer) override;

	/** Respawn a player after they die */
	UFUNCTION()
	void RespawnPlayer(AController* Controller);

protected:
	/** Time to wait before respawning (seconds) */
	UPROPERTY(EditDefaultsOnly, Category = "Respawn")
	float RespawnDelay = 3.0f;

	/** Map of controllers waiting to respawn and their timer handles */
	TMap<AController*, FTimerHandle> PendingRespawns;

	/** Internal function called after delay to actually respawn */
	UFUNCTION()
	void RespawnPlayer_Internal(AController* Controller);
};



