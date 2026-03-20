#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "MatchSessionInfo.h"
#include "SteamSessionManager.generated.h"

class UMatchmakingSubsystem;

// Delegate for when Steam session is created
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSteamSessionCreated, bool, bSuccess, const FString&, SessionName);

// Delegate for when Steam session search completes
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSteamSessionsFound, bool, bSuccess, int32, NumResults);

// Delegate for when Steam session join completes
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSteamSessionJoined, bool, bSuccess, const FString&, HostSteamId);

// Delegate for when Steam session is destroyed
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSteamSessionDestroyed, bool, bSuccess);

/**
 * Game Instance Subsystem for managing Steam P2P sessions.
 * Handles creating, finding, joining, and leaving Steam sessions.
 * Uses the existing TCP matchmaking server for session registration/discovery.
 */
UCLASS()
class EXAMPLEPROJECT_API USteamSessionManager : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    /**
     * Create a new Steam P2P session, register it with the matchmaking server,
     * and open a listen server on the specified map so clients can connect.
     *
     * @param SessionName  Display name for the session
     * @param MaxPlayers   Maximum number of players
     * @param InGameMode   Game mode tag for filtering
     * @param InMapName    Map to open as listen server (e.g. "/Game/ThirdPerson/Lvl_ThirdPerson")
     * @param bIsPrivate   Whether the session should be private
     */
    UFUNCTION(BlueprintCallable, Category = "Steam|Session")
    void CreateSteamSession(const FString& SessionName, int32 MaxPlayers = 8,
        const FString& InGameMode = TEXT("Default"),
        const FString& InMapName = TEXT("/Game/ThirdPerson/Lvl_ThirdPerson"),
        bool bIsPrivate = false);

    // --- Session Discovery ---

    /** Find Steam sessions via the Steam network */
    UFUNCTION(BlueprintCallable, Category = "Steam|Session")
    void FindSteamSessions(const FString& GameModeFilter = TEXT(""), int32 MaxResults = 50);

    /** Join a Steam session by its search result index */
    UFUNCTION(BlueprintCallable, Category = "Steam|Session")
    void JoinSteamSessionByIndex(int32 SessionIndex);

    /** Direct connect to a Steam host by their Steam ID (fallback) */
    UFUNCTION(BlueprintCallable, Category = "Steam|Session")
    void ClientTravelToSteamHost(const FString& HostSteamId);

    /** Join the appropriate session based on session type (dedicated vs steam_p2p) */
    UFUNCTION(BlueprintCallable, Category = "Steam|Session")
    void JoinSessionByInfo(const FMatchSessionInfo& SessionInfo);

    // --- Session Management ---

    /** Leave the current Steam session */
    UFUNCTION(BlueprintCallable, Category = "Steam|Session")
    void LeaveSteamSession();

    /** Register the current Steam session with the matchmaking server via TCP */
    UFUNCTION(BlueprintCallable, Category = "Steam|Session")
    void RegisterSessionWithBackend();

    /** Unregister the current session from the matchmaking server via TCP */
    UFUNCTION(BlueprintCallable, Category = "Steam|Session")
    void UnregisterSessionFromBackend();

    /** Manually send a heartbeat (auto-heartbeat is started on session create) */
    UFUNCTION(BlueprintCallable, Category = "Steam|Session")
    void SendHeartbeat();

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Steam|Session")
    bool IsHosting() const { return bIsHosting; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Steam|Session")
    bool IsInSession() const { return bIsInSession; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Steam|Session")
    FString GetLocalSteamId() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Steam|Session")
    FString GetLocalSteamName() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Steam|Session")
    const FSteamHostedSession& GetHostedSession() const { return HostedSession; }

    // --- Delegates ---

    UPROPERTY(BlueprintAssignable, Category = "Steam|Session")
    FOnSteamSessionCreated OnSteamSessionCreated;

    UPROPERTY(BlueprintAssignable, Category = "Steam|Session")
    FOnSteamSessionsFound OnSteamSessionsFound;

    UPROPERTY(BlueprintAssignable, Category = "Steam|Session")
    FOnSteamSessionJoined OnSteamSessionJoined;

    UPROPERTY(BlueprintAssignable, Category = "Steam|Session")
    FOnSteamSessionDestroyed OnSteamSessionDestroyed;

private:
    void OnCreateSessionComplete(FName InSessionName, bool bWasSuccessful);
    void OnFindSessionsComplete(bool bWasSuccessful);
    void OnJoinSessionComplete(FName InSessionName, EOnJoinSessionCompleteResult::Type Result);
    void OnDestroySessionComplete(FName InSessionName, bool bWasSuccessful);

    UFUNCTION()
    void OnBackendRegistered(bool bSuccess, const FString& SessionId);

    void StartHeartbeatTimer();
    void StopHeartbeatTimer();
    void OnHeartbeatTick();

    UMatchmakingSubsystem* GetMatchmakingSubsystem() const;

    bool bIsHosting = false;
    bool bIsInSession = false;
    FSteamHostedSession HostedSession;

    FString PendingSessionName;
    FString PendingGameMode;
    FString PendingMapName;
    int32 PendingMaxPlayers = 8;
    bool PendingIsPrivate = false;

    TSharedPtr<FOnlineSessionSearch> SessionSearch;
    FTimerHandle HeartbeatTimerHandle;
    float HeartbeatInterval = 30.0f;

    FDelegateHandle CreateSessionDelegateHandle;
    FDelegateHandle FindSessionsDelegateHandle;
    FDelegateHandle JoinSessionDelegateHandle;
    FDelegateHandle DestroySessionDelegateHandle;
};
