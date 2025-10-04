// Fill out your copyright notice in the Description page of Project Settings.

#include "GameManager.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "BasePickup.h"

// Initialize static instance
AGameManager* AGameManager::Instance = nullptr;

// Sets default values
AGameManager::AGameManager()
{
 	// Set this actor to call Tick() every frame
	PrimaryActorTick.bCanEverTick = true;

	// Set as singleton
	Instance = this;
}

// Called when the game starts or when spawned
void AGameManager::BeginPlay()
{
	Super::BeginPlay();
	
	// Count total pickups in the level
	TArray<AActor*> FoundPickups;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABasePickup::StaticClass(), FoundPickups);
	
	TotalPickups = FoundPickups.Num();
	UpdateCompletionPercentage();

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::White, 
			FString::Printf(TEXT("Found %d pickups in level"), TotalPickups));
	}
}

// Called every frame
void AGameManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Add score
void AGameManager::AddScore(int32 Points)
{
	CurrentScore += Points;
	OnScoreUpdated.Broadcast(CurrentScore);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, 
			FString::Printf(TEXT("Score: %d"), CurrentScore));
	}
}

// Add resource
void AGameManager::AddResource(int32 ResourceValue)
{
	TotalResources += ResourceValue;
	OnResourceCollected.Broadcast(ResourceValue);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Blue, 
			FString::Printf(TEXT("Resources: %d"), TotalResources));
	}
}

// On pickup collected
void AGameManager::OnPickupCollected(int32 ResourceValue)
{
	PickupsCollected++;
	AddScore(ResourceValue * 10); // 10 points per resource
	AddResource(ResourceValue);
	UpdateCompletionPercentage();

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Yellow, 
			FString::Printf(TEXT("Pickup %d/%d collected! (%d%% complete)"), 
				PickupsCollected, TotalPickups, FMath::RoundToInt(CompletionPercentage)));
	}

	// Check if game is completed
	if (PickupsCollected >= TotalPickups && TotalPickups > 0)
	{
		OnGameCompleted.Broadcast(CompletionPercentage);
		
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, 
				TEXT("🎉 GAME COMPLETED! All pickups collected!"));
		}
	}
}

// Register pickup
void AGameManager::RegisterPickup()
{
	TotalPickups++;
	UpdateCompletionPercentage();
}

// Update completion percentage
void AGameManager::UpdateCompletionPercentage()
{
	if (TotalPickups > 0)
	{
		CompletionPercentage = (float)PickupsCollected / (float)TotalPickups * 100.0f;
	}
	else
	{
		CompletionPercentage = 0.0f;
	}
}


// Get game manager instance
AGameManager* AGameManager::GetGameManager(const UObject* WorldContext)
{
	if (Instance)
	{
		return Instance;
	}

	// Try to find existing instance in world
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::LogAndReturnNull))
	{
		TArray<AActor*> FoundManagers;
		UGameplayStatics::GetAllActorsOfClass(World, AGameManager::StaticClass(), FoundManagers);
		
		if (FoundManagers.Num() > 0)
		{
			Instance = Cast<AGameManager>(FoundManagers[0]);
		}
	}

	return Instance;
}
