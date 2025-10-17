// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "MyPlayerState.generated.h"

UENUM(BlueprintType)
enum class EHostingType : uint8
{
    DedicatedServer    UMETA(DisplayName = "Dedicated Server"),
    ClientHost         UMETA(DisplayName = "Client Host")
};

USTRUCT()
struct FSessionInfo {
    GENERATED_BODY()

    UPROPERTY()
    int id;

    UPROPERTY()
    FString name;

    UPROPERTY()
    FString serverip;

    UPROPERTY()
    int serverport;

    UPROPERTY()
    EHostingType hostingType;
};

/**
 * 
 */
UCLASS()
class EXAMPLEPROJECT_API AMyPlayerState : public APlayerState
{
	GENERATED_BODY()

public:

	UFUNCTION()
	void AddCoin();

private:

    UPROPERTY(Replicated, VisibleAnywhere, Category = "Coins")
    int collectedCoins;
	
	
};
