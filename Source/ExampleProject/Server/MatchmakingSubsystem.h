// Fill out your copyright notice in the Description page of Project Settings.
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MatchSessionInfo.h"
#include "MatchmakingSubsystem.generated.h"

class FTCPClientRunnable;

// Delegate for when session list is updated
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSessionsUpdated, const TArray<FMatchSessionInfo>&, Sessions);

// Delegate for when host request is confirmed by server
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnHostRequested, int32, SessionId, FString, ServerIp, int32, ServerPort);

// Delegate for when connection status changes
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnConnectionStatusChanged, bool, bIsConnected);

// Delegate for when join is successful
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnJoinSuccess, int32, SessionId, FString, ServerIp, int32, ServerPort);

// Delegate for when disconnect is confirmed
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDisconnectSuccess);

// Delegate for when session shutdown is confirmed
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnShutdownSuccess, int32, SessionId);

// Delegate for when server sends an error
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnServerError, FString, ErrorCode);

/**
 * Game Instance Subsystem for managing matchmaking connection
 */
UCLASS()
class EXAMPLEPROJECT_API UMatchmakingSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    // Subsystem lifecycle
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // Connect to matchmaking server
    UFUNCTION(BlueprintCallable, Category = "Matchmaking")
    void ConnectToMatchmakingServer();

    // Host a new game session
    UFUNCTION(BlueprintCallable, Category = "Matchmaking")
    void HostNewGame(const FString& Name);

    // Refresh the session list
    UFUNCTION(BlueprintCallable, Category = "Matchmaking")
    void RefreshSessionList();

    // Join an existing session
    UFUNCTION(BlueprintCallable, Category = "Matchmaking")
    void JoinSession(int32 SessionId);

    // Disconnect from current session
    UFUNCTION(BlueprintCallable, Category = "Matchmaking")
    void DisconnectFromSession();

    // Shutdown a session (only host can do this)
    UFUNCTION(BlueprintCallable, Category = "Matchmaking")
    void ShutdownSession(int32 SessionId);

    // Check if connected to matchmaking server
    UFUNCTION(BlueprintCallable, Category = "Matchmaking")
    bool IsConnected() const;

    // Get current session list
    UFUNCTION(BlueprintCallable, Category = "Matchmaking")
    const TArray<FMatchSessionInfo>& GetSessions() const { return Sessions; }

    // Called by TCP runnable when server message is received
    void HandleServerMessage(const FString& ServerMessage);

    // Called by TCP runnable when connection status changes
    void HandleConnectionStatusChanged(bool bIsConnected);

    // Delegates
    UPROPERTY(BlueprintAssignable, Category = "Matchmaking")
    FOnSessionsUpdated OnSessionsUpdated;

    UPROPERTY(BlueprintAssignable, Category = "Matchmaking")
    FOnHostRequested OnHostRequested;

    UPROPERTY(BlueprintAssignable, Category = "Matchmaking")
    FOnConnectionStatusChanged OnConnectionStatusChanged;

    UPROPERTY(BlueprintAssignable, Category = "Matchmaking")
    FOnJoinSuccess OnJoinSuccess;

    UPROPERTY(BlueprintAssignable, Category = "Matchmaking")
    FOnDisconnectSuccess OnDisconnectSuccess;

    UPROPERTY(BlueprintAssignable, Category = "Matchmaking")
    FOnShutdownSuccess OnShutdownSuccess;

    UPROPERTY(BlueprintAssignable, Category = "Matchmaking")
    FOnServerError OnServerError;

private:
    // Parse session list from server message
    void ParseAndSetSessions(const FString& ServerMessage);

    FTCPClientRunnable* ClientRunnable;
    TArray<FMatchSessionInfo> Sessions;

    // Track current session info
    int32 CurrentSessionId = -1;
};

