// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MatchSessionInfo.h"
#include "MatchmakingSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSessionsUpdated, const TArray<FMatchSessionInfo>&, Sessions);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHostRequested, int32, Port);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnConnectionStatusChanged, bool, bIsConnected);

class FTCPClientRunnable;

UCLASS()
class EXAMPLEPROJECT_API UMatchmakingSubsystem : public UGameInstanceSubsystem {
    GENERATED_BODY()

public:
    // Subsystem lifecycle
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // Public API
    UFUNCTION(BlueprintCallable)
    void ConnectToMatchmakingServer();

    UFUNCTION(BlueprintCallable)
    void HostNewGame(const FString& Name, int32 Port);

    UFUNCTION(BlueprintCallable)
    void RefreshSessionList();

    UFUNCTION(BlueprintCallable)
    bool IsConnected() const;

    const TArray<FMatchSessionInfo>& GetSessions() const { return Sessions; }

    UPROPERTY(BlueprintAssignable)
    FOnSessionsUpdated OnSessionsUpdated;

    UPROPERTY(BlueprintAssignable)
    FOnHostRequested OnHostRequested;

    UPROPERTY(BlueprintAssignable)
    FOnConnectionStatusChanged OnConnectionStatusChanged;

    // Called by the runnable on the game thread
    void HandleServerMessage(const FString& ServerMessage);
    
    // Called by the runnable when connection status changes
    void HandleConnectionStatusChanged(bool bIsConnected);

private:
    // Owned runnable that talks to the matchmaking server
    FTCPClientRunnable* ClientRunnable = nullptr;
    // Stored sessions (value types)
    TArray<FMatchSessionInfo> Sessions;

    // Helpers
    void ParseAndSetSessions(const FString& ServerMessage);
};
