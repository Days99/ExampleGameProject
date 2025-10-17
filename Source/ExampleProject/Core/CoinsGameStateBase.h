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

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly)
	int TotalLevelCoins;

	// Puzzle System - Pressure Plates
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle")
	TArray<class APressurePlate*> PressurePlates;

	// Replicated puzzle completion state with RepNotify
	UPROPERTY(ReplicatedUsing=OnRep_PuzzleCompleted, VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle")
	bool bPuzzleCompleted;

	// Check if puzzle is complete (all plates active + all coins collected)
	UFUNCTION(BlueprintPure, Category = "Puzzle")
	bool IsPuzzleComplete() const { return bPuzzleCompleted; }

	UFUNCTION(NetMulticast, Reliable)
	void MulticastOnLevelComplete(APawn* character, bool succeeded);

	UFUNCTION(BlueprintImplementableEvent, Category = "Gameplay Events")
	void OnLevelCompleted(APawn* character, bool succeeded);

protected:
	// RepNotify for puzzle completion
	UFUNCTION()
	void OnRep_PuzzleCompleted();

	// Check puzzle state on server
	void CheckPuzzleState();

	// Find all pressure plates in the level
	void FindPressurePlates();

public:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;



	
};
