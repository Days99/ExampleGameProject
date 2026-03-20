#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MatchSessionInfo.h"
#include "MatchmakingSubsystem.generated.h"

class FTCPClientRunnable;
class USteamSessionManager;

// Delegate for when session list is updated
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMatchmakingOnSessionsUpdated, const TArray<FMatchSessionInfo>&, Sessions);

// Delegate for when host request is confirmed by server
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FMatchmakingOnHostRequested, int32, SessionId, FString, ServerIp, int32, ServerPort);

// Delegate for when connection status changes
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMatchmakingOnConnectionStatusChanged, bool, bIsConnected);

// Delegate for when join is successful
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FMatchmakingOnJoinSuccess, int32, SessionId, FString, ServerIp, int32, ServerPort);

// Delegate for when disconnect is confirmed
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMatchmakingOnDisconnectSuccess);

// Delegate for when session shutdown is confirmed
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMatchmakingOnShutdownSuccess, int32, SessionId);

// Delegate for when server sends an error
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMatchmakingOnServerError, FString, ErrorCode);

// Delegate for when Steam P2P session registration response is received
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMatchmakingOnSteamP2PRegistered, bool, bSuccess, const FString&, SessionId);

// Delegate for when Steam P2P session unregistration response is received
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMatchmakingOnSteamP2PUnregistered, bool, bSuccess);

/**
 * Game Instance Subsystem for managing matchmaking connection.
 * All communication goes through a single TCP connection to the matchmaking server.
 */
UCLASS()
class EXAMPLEPROJECT_API UMatchmakingSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category = "Matchmaking")
    void ConnectToMatchmakingServer();

    UFUNCTION(BlueprintCallable, Category = "Matchmaking")
    void HostNewGame(const FString& Name);

    UFUNCTION(BlueprintCallable, Category = "Matchmaking")
    void RefreshSessionList();

    UFUNCTION(BlueprintCallable, Category = "Matchmaking")
    void JoinSession(int32 SessionId);

    UFUNCTION(BlueprintCallable, Category = "Matchmaking")
    void DisconnectFromSession();

    UFUNCTION(BlueprintCallable, Category = "Matchmaking")
    void ShutdownSession(int32 SessionId);

    /** Register a Steam P2P session with the matchmaking server */
    UFUNCTION(BlueprintCallable, Category = "Matchmaking|Steam")
    void RegisterSteamP2PSession(const FString& SessionName, const FString& HostSteamId,
        const FString& HostPlayerName, const FString& GameMode, int32 MaxPlayers,
        const FString& SteamLobbyId, bool bIsPrivate);

    /** Send heartbeat for an active Steam P2P session */
    UFUNCTION(BlueprintCallable, Category = "Matchmaking|Steam")
    void SendSteamHeartbeat(const FString& SessionId, int32 CurrentPlayers, const FString& MapName);

    /** Unregister a Steam P2P session from the matchmaking server */
    UFUNCTION(BlueprintCallable, Category = "Matchmaking|Steam")
    void UnregisterSteamP2PSession(const FString& SessionId, const FString& HostSteamId);

    UFUNCTION(BlueprintCallable, Category = "Matchmaking")
    bool IsConnected() const;

    UFUNCTION(BlueprintCallable, Category = "Matchmaking")
    const TArray<FMatchSessionInfo>& GetSessions() const { return Sessions; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Matchmaking|Steam")
    USteamSessionManager* GetSteamSessionManager() const;

    /** Join a session based on its type (handles both dedicated and steam_p2p) */
    UFUNCTION(BlueprintCallable, Category = "Matchmaking")
    void JoinSessionByInfo(const FMatchSessionInfo& SessionInfo);

    void HandleServerMessage(const FString& ServerMessage);
    void HandleConnectionStatusChanged(bool bIsConnected);

    UPROPERTY(BlueprintAssignable, Category = "Matchmaking")
    FMatchmakingOnSessionsUpdated OnSessionsUpdated;

    UPROPERTY(BlueprintAssignable, Category = "Matchmaking")
    FMatchmakingOnHostRequested OnHostRequested;

    UPROPERTY(BlueprintAssignable, Category = "Matchmaking")
    FMatchmakingOnConnectionStatusChanged OnConnectionStatusChanged;

    UPROPERTY(BlueprintAssignable, Category = "Matchmaking")
    FMatchmakingOnJoinSuccess OnJoinSuccess;

    UPROPERTY(BlueprintAssignable, Category = "Matchmaking")
    FMatchmakingOnDisconnectSuccess OnDisconnectSuccess;

    UPROPERTY(BlueprintAssignable, Category = "Matchmaking")
    FMatchmakingOnShutdownSuccess OnShutdownSuccess;

    UPROPERTY(BlueprintAssignable, Category = "Matchmaking")
    FMatchmakingOnServerError OnServerError;

    UPROPERTY(BlueprintAssignable, Category = "Matchmaking|Steam")
    FMatchmakingOnSteamP2PRegistered OnSteamP2PRegistered;

    UPROPERTY(BlueprintAssignable, Category = "Matchmaking|Steam")
    FMatchmakingOnSteamP2PUnregistered OnSteamP2PUnregistered;

private:
    void ParseAndSetSessions(const FString& ServerMessage);

    FTCPClientRunnable* ClientRunnable;
    TArray<FMatchSessionInfo> Sessions;
    int32 CurrentSessionId = -1;
};
