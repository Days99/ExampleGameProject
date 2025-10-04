// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "CoinsGameStateBase.generated.h"

/**
 * 
 */
UCLASS()
class EXAMPLEPROJECT_API ACoinsGameStateBase : public AGameStateBase
{
	GENERATED_BODY()

public:

	ACoinsGameStateBase();
	void UpdateTotalCoinsInLevel();

	UPROPERTY(Replicated, VisibleAnywhere,BlueprintReadOnly)
	int TotalLevelCoins;

	UFUNCTION(NetMulticast, Reliable)
	void MulticastOnLevelComplete(APawn* character, bool succeeded);

	UFUNCTION(BlueprintImplementableEvent, Category = "Gameplay Events")
	void OnLevelCompleted(APawn* character, bool succeeded);



	
};
