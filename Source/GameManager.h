// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/Engine.h"
#include "GameManager.generated.h"

/**
 * Game Manager class to handle game state, scoring, and event coordination
 * Manages pickup collection, score tracking, and UI updates
 */
UCLASS()
class MYPROJECT_API AGameManager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AGameManager();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Current score
	UPROPERTY(BlueprintReadOnly, Category = "Game State")
	int32 CurrentScore = 0;

	// Total resources collected
	UPROPERTY(BlueprintReadOnly, Category = "Game State")
	int32 TotalResources = 0;

	// Number of pickups in the level
	UPROPERTY(BlueprintReadOnly, Category = "Game State")
	int32 TotalPickups = 0;

	// Pickups collected
	UPROPERTY(BlueprintReadOnly, Category = "Game State")
	int32 PickupsCollected = 0;

	// Game completion percentage
	UPROPERTY(BlueprintReadOnly, Category = "Game State")
	float CompletionPercentage = 0.0f;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Score management
	UFUNCTION(BlueprintCallable, Category = "Game State")
	void AddScore(int32 Points);

	UFUNCTION(BlueprintCallable, Category = "Game State")
	void AddResource(int32 ResourceValue);

	UFUNCTION(BlueprintCallable, Category = "Game State")
	void OnPickupCollected(int32 ResourceValue);

	// Game state management
	UFUNCTION(BlueprintCallable, Category = "Game State")
	void RegisterPickup();

	UFUNCTION(BlueprintCallable, Category = "Game State")
	void UpdateCompletionPercentage();

	// Getters
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Game State")
	int32 GetCurrentScore() const { return CurrentScore; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Game State")
	int32 GetTotalResources() const { return TotalResources; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Game State")
	int32 GetPickupsCollected() const { return PickupsCollected; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Game State")
	int32 GetTotalPickups() const { return TotalPickups; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Game State")
	float GetCompletionPercentage() const { return CompletionPercentage; }

	// Event dispatchers
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnScoreUpdated, int32, NewScore);
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnScoreUpdated OnScoreUpdated;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnResourceCollected, int32, ResourceValue);
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnResourceCollected OnResourceCollected;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGameCompleted, float, CompletionPercentage);
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnGameCompleted OnGameCompleted;

	// Static function to get the game manager instance
	UFUNCTION(BlueprintCallable, Category = "Game State")
	static AGameManager* GetGameManager(const UObject* WorldContext);

private:
	// Singleton instance
	static AGameManager* Instance;
};

