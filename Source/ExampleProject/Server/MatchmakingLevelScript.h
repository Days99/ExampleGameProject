// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/LevelScriptActor.h"
#include "MatchSessionInfo.h"
#include "MatchmakingLevelScript.generated.h"

class UUserWidget;
class UMatchmakingSubsystem;
class UScrollBox;

/**
 * Level script for managing matchmaking UI and interactions
 */
UCLASS()
class AMatchmakingLevelScript : public ALevelScriptActor
{
    GENERATED_BODY()

public:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // Widget class to spawn
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Matchmaking")
    TSubclassOf<UUserWidget> MatchmakingWidgetClass;

    // Auto-refresh interval in seconds
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Matchmaking")
    float RefreshInterval = 5.0f;

protected:
    // Button callbacks
    UFUNCTION()
    void OnConnectClicked();

    UFUNCTION()
    void OnHostClicked();

    UFUNCTION()
    void OnJoinSessionClicked(int32 SessionId);

    // Delegate callbacks from subsystem
    UFUNCTION()
    void OnSessionsUpdated(const TArray<FMatchSessionInfo>& Sessions);

    UFUNCTION()
    void OnHostRequested(int32 SessionId, FString ServerIp, int32 ServerPort);

    UFUNCTION()
    void OnConnectionStatusChanged(bool bIsConnected);

    UFUNCTION()
    void OnJoinSuccess(int32 SessionId, FString ServerIp, int32 ServerPort);

    UFUNCTION()
    void OnServerError(FString ErrorCode);

    // Timer callback
    UFUNCTION()
    void RefreshSessionList();

    // UI rebuild
    void RebuildServerListUI();

private:
    UPROPERTY()
    UUserWidget* MatchmakingWidget;

    UPROPERTY()
    UMatchmakingSubsystem* MatchSubsystem;

    UPROPERTY()
    UScrollBox* ServerListScrollBoxWidget;

    FTimerHandle RefreshTimerHandle;
};

