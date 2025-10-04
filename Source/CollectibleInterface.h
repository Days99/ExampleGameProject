// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CollectibleInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI, Blueprintable)
class UCollectibleInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Interface for collectible objects
 * Allows any actor to be collected by implementing this interface
 */
class MYPROJECT_API ICollectibleInterface
{
	GENERATED_BODY()

public:
	// Function to collect the object
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Collectible")
	void Collect();

	// Function to check if object can be collected
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Collectible")
	bool CanBeCollected() const;
};

