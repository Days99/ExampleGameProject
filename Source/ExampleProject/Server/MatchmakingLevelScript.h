// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/LevelScriptActor.h"
#include "MatchmakingSubsystem.h"
#include "Server/MatchSessionInfo.h"
#include <Components/ScrollBox.h>
#include "MatchmakingLevelScript.generated.h"

/**
 * 
 */
UCLASS()
class EXAMPLEPROJECT_API AMatchmakingLevelScript : public ALevelScriptActor
{
    GENERATED_BODY()

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UFUNCTION()
    void OnConnectClicked();

    UFUNCTION()
    void RefreshSessionList();

    UPROPERTY()
    UMatchmakingSubsystem* MatchSubsystem;
    UFUNCTION()
    void OnHostClicked();

    UFUNCTION()
    void OnSessionsUpdated(const TArray<FMatchSessionInfo>& Sessions);

    UFUNCTION()
    void OnHostRequested(int32 Port);

    UFUNCTION()
    void OnConnectionStatusChanged(bool bIsConnected);

    UPROPERTY(EditDefaultsOnly, Category = "UI")  
    TSubclassOf<class UUserWidget> MatchmakingWidgetClass;

    // UI widgets references (as before)
    UUserWidget* MatchmakingWidget;
    UScrollBox* ServerListScrollBoxWidget;

    void RebuildServerListUI();

    // Refresh settings
    UPROPERTY(EditDefaultsOnly, Category = "Matchmaking")
    float RefreshInterval = 3.0f; // Refresh every 3 seconds

    // Timer handle for session refresh
    FTimerHandle RefreshTimerHandle;
};
