#include "Server/SteamSessionManager.h"
#include "Server/MatchmakingSubsystem.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

void USteamSessionManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    UE_LOG(LogTemp, Log, TEXT("SteamSessionManager initialized (TCP-only)"));
}

void USteamSessionManager::Deinitialize()
{
    if (bIsHosting && !HostedSession.SessionId.IsEmpty())
    {
        UnregisterSessionFromBackend();
    }

    StopHeartbeatTimer();

    if (bIsInSession)
    {
        IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get(STEAM_SUBSYSTEM);
        if (OnlineSub)
        {
            IOnlineSessionPtr SessionInterface = OnlineSub->GetSessionInterface();
            if (SessionInterface.IsValid())
            {
                SessionInterface->DestroySession(NAME_GameSession);
            }
        }
    }

    if (UMatchmakingSubsystem* MatchSub = GetMatchmakingSubsystem())
    {
        MatchSub->OnSteamP2PRegistered.RemoveDynamic(this, &USteamSessionManager::OnBackendRegistered);
    }

    Super::Deinitialize();
}

UMatchmakingSubsystem* USteamSessionManager::GetMatchmakingSubsystem() const
{
    if (UGameInstance* GI = GetGameInstance())
    {
        return GI->GetSubsystem<UMatchmakingSubsystem>();
    }
    return nullptr;
}

void USteamSessionManager::CreateSteamSession(const FString& SessionName, int32 MaxPlayers,
    const FString& InGameMode, const FString& InMapName, bool bIsPrivate)
{
    IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get(STEAM_SUBSYSTEM);
    if (!OnlineSub)
    {
        UE_LOG(LogTemp, Error, TEXT("SteamSessionManager: Steam Online Subsystem not available!"));
        OnSteamSessionCreated.Broadcast(false, SessionName);
        return;
    }

    IOnlineSessionPtr SessionInterface = OnlineSub->GetSessionInterface();
    if (!SessionInterface.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("SteamSessionManager: Session interface not valid!"));
        OnSteamSessionCreated.Broadcast(false, SessionName);
        return;
    }

    PendingSessionName = SessionName;
    PendingGameMode = InGameMode;
    PendingMapName = InMapName;
    PendingMaxPlayers = MaxPlayers;
    PendingIsPrivate = bIsPrivate;

    auto ExistingSession = SessionInterface->GetNamedSession(NAME_GameSession);
    if (ExistingSession != nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("SteamSessionManager: Existing session found, destroying first..."));
        DestroySessionDelegateHandle = SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(
            FOnDestroySessionCompleteDelegate::CreateUObject(this, &USteamSessionManager::OnDestroySessionComplete));
        SessionInterface->DestroySession(NAME_GameSession);
        return;
    }

    FOnlineSessionSettings SessionSettings;
    SessionSettings.bIsLANMatch = false;
    SessionSettings.NumPublicConnections = MaxPlayers;
    SessionSettings.bShouldAdvertise = !bIsPrivate;
    SessionSettings.bUsesPresence = true;
    SessionSettings.bAllowJoinInProgress = true;
    SessionSettings.bAllowJoinViaPresence = true;
    SessionSettings.bUseLobbiesIfAvailable = true;

    FString LocalSteamId = GetLocalSteamId();
    FString LocalSteamName = GetLocalSteamName();

    SessionSettings.Set(FName("HostSteamId"), LocalSteamId, EOnlineDataAdvertisementType::ViaOnlineService);
    SessionSettings.Set(FName("HostPlayerName"), LocalSteamName, EOnlineDataAdvertisementType::ViaOnlineService);
    SessionSettings.Set(FName("GameMode"), InGameMode, EOnlineDataAdvertisementType::ViaOnlineService);
    SessionSettings.Set(FName("MapName"), InMapName, EOnlineDataAdvertisementType::ViaOnlineService);
    SessionSettings.Set(FName("SessionName"), SessionName, EOnlineDataAdvertisementType::ViaOnlineService);

    CreateSessionDelegateHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(
        FOnCreateSessionCompleteDelegate::CreateUObject(this, &USteamSessionManager::OnCreateSessionComplete));

    UE_LOG(LogTemp, Log, TEXT("SteamSessionManager: Creating Steam session '%s' (Max: %d, Mode: %s, Map: %s)"),
        *SessionName, MaxPlayers, *InGameMode, *InMapName);

    SessionInterface->CreateSession(0, NAME_GameSession, SessionSettings);
}

void USteamSessionManager::OnCreateSessionComplete(FName InSessionName, bool bWasSuccessful)
{
    IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get(STEAM_SUBSYSTEM);
    if (OnlineSub)
    {
        IOnlineSessionPtr SessionInterface = OnlineSub->GetSessionInterface();
        if (SessionInterface.IsValid())
        {
            SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionDelegateHandle);
        }
    }

    if (bWasSuccessful)
    {
        bIsHosting = true;
        bIsInSession = true;

        HostedSession.SessionName = PendingSessionName;
        HostedSession.HostSteamId = GetLocalSteamId();
        HostedSession.HostPlayerName = GetLocalSteamName();
        HostedSession.GameMode = PendingGameMode;
        HostedSession.MapName = PendingMapName;
        HostedSession.MaxPlayers = PendingMaxPlayers;
        HostedSession.bIsPrivate = PendingIsPrivate;
        HostedSession.CurrentPlayers = 1;

        UE_LOG(LogTemp, Log, TEXT("SteamSessionManager: Session '%s' created successfully! Host: %s (%s)"),
            *PendingSessionName, *HostedSession.HostPlayerName, *HostedSession.HostSteamId);

        RegisterSessionWithBackend();
        StartHeartbeatTimer();

        if (UWorld* World = GetGameInstance()->GetWorld())
        {
            if (APlayerController* PC = World->GetFirstPlayerController())
            {
                FString TravelURL = FString::Printf(TEXT("%s?listen"), *PendingMapName);
                UE_LOG(LogTemp, Warning, TEXT("SteamSessionManager: Opening listen server - %s"), *TravelURL);
                PC->ClientTravel(TravelURL, ETravelType::TRAVEL_Absolute);
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("SteamSessionManager: No PlayerController for listen server travel!"));
            }
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("SteamSessionManager: Failed to create session '%s'"), *PendingSessionName);
    }

    OnSteamSessionCreated.Broadcast(bWasSuccessful, PendingSessionName);
}

void USteamSessionManager::FindSteamSessions(const FString& GameModeFilter, int32 MaxResults)
{
    IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get(STEAM_SUBSYSTEM);
    if (!OnlineSub)
    {
        UE_LOG(LogTemp, Error, TEXT("SteamSessionManager: Steam not available for session search"));
        OnSteamSessionsFound.Broadcast(false, 0);
        return;
    }

    IOnlineSessionPtr SessionInterface = OnlineSub->GetSessionInterface();
    if (!SessionInterface.IsValid())
    {
        OnSteamSessionsFound.Broadcast(false, 0);
        return;
    }

    SessionSearch = MakeShareable(new FOnlineSessionSearch());
    SessionSearch->MaxSearchResults = MaxResults;
    SessionSearch->bIsLanQuery = false;
    SessionSearch->QuerySettings.Set(FName(TEXT("PRESENCESEARCH")), true, EOnlineComparisonOp::Equals);

    if (!GameModeFilter.IsEmpty())
    {
        SessionSearch->QuerySettings.Set(FName("GameMode"), GameModeFilter, EOnlineComparisonOp::Equals);
    }

    FindSessionsDelegateHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(
        FOnFindSessionsCompleteDelegate::CreateUObject(this, &USteamSessionManager::OnFindSessionsComplete));

    UE_LOG(LogTemp, Log, TEXT("SteamSessionManager: Searching for Steam sessions (Max: %d)"), MaxResults);
    SessionInterface->FindSessions(0, SessionSearch.ToSharedRef());
}

void USteamSessionManager::OnFindSessionsComplete(bool bWasSuccessful)
{
    IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get(STEAM_SUBSYSTEM);
    if (OnlineSub)
    {
        IOnlineSessionPtr SessionInterface = OnlineSub->GetSessionInterface();
        if (SessionInterface.IsValid())
        {
            SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsDelegateHandle);
        }
    }

    int32 NumResults = 0;
    if (bWasSuccessful && SessionSearch.IsValid())
    {
        NumResults = SessionSearch->SearchResults.Num();
        UE_LOG(LogTemp, Log, TEXT("SteamSessionManager: Found %d Steam sessions"), NumResults);

        for (int32 i = 0; i < NumResults; i++)
        {
            const FOnlineSessionSearchResult& Result = SessionSearch->SearchResults[i];
            FString HostName, MapName, GameMode;
            Result.Session.SessionSettings.Get(FName("HostPlayerName"), HostName);
            Result.Session.SessionSettings.Get(FName("MapName"), MapName);
            Result.Session.SessionSettings.Get(FName("GameMode"), GameMode);
            UE_LOG(LogTemp, Log, TEXT("  [%d] Host: %s, Map: %s, Mode: %s, OpenSlots: %d"),
                i, *HostName, *MapName, *GameMode, Result.Session.NumOpenPublicConnections);
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("SteamSessionManager: Session search failed"));
    }

    OnSteamSessionsFound.Broadcast(bWasSuccessful, NumResults);
}

void USteamSessionManager::JoinSteamSessionByIndex(int32 SessionIndex)
{
    if (!SessionSearch.IsValid() || !SessionSearch->SearchResults.IsValidIndex(SessionIndex))
    {
        UE_LOG(LogTemp, Error, TEXT("SteamSessionManager: Invalid session index %d"), SessionIndex);
        OnSteamSessionJoined.Broadcast(false, TEXT(""));
        return;
    }

    IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get(STEAM_SUBSYSTEM);
    if (!OnlineSub)
    {
        OnSteamSessionJoined.Broadcast(false, TEXT(""));
        return;
    }

    IOnlineSessionPtr SessionInterface = OnlineSub->GetSessionInterface();
    if (!SessionInterface.IsValid())
    {
        OnSteamSessionJoined.Broadcast(false, TEXT(""));
        return;
    }

    JoinSessionDelegateHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(
        FOnJoinSessionCompleteDelegate::CreateUObject(this, &USteamSessionManager::OnJoinSessionComplete));

    UE_LOG(LogTemp, Log, TEXT("SteamSessionManager: Joining Steam session at index %d"), SessionIndex);
    SessionInterface->JoinSession(0, NAME_GameSession, SessionSearch->SearchResults[SessionIndex]);
}

void USteamSessionManager::OnJoinSessionComplete(FName InSessionName, EOnJoinSessionCompleteResult::Type Result)
{
    IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get(STEAM_SUBSYSTEM);
    FString ConnectString;

    if (OnlineSub)
    {
        IOnlineSessionPtr SessionInterface = OnlineSub->GetSessionInterface();
        if (SessionInterface.IsValid())
        {
            SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionDelegateHandle);
            SessionInterface->GetResolvedConnectString(NAME_GameSession, ConnectString);
        }
    }

    bool bSuccess = (Result == EOnJoinSessionCompleteResult::Success);

    if (bSuccess)
    {
        bIsInSession = true;
        UE_LOG(LogTemp, Log, TEXT("SteamSessionManager: Joined session! Connect string: %s"), *ConnectString);

        if (!ConnectString.IsEmpty())
        {
            if (UWorld* World = GetGameInstance()->GetWorld())
            {
                if (APlayerController* PC = World->GetFirstPlayerController())
                {
                    UE_LOG(LogTemp, Warning, TEXT("SteamSessionManager: Traveling to host via: %s"), *ConnectString);
                    PC->ClientTravel(ConnectString, ETravelType::TRAVEL_Absolute);
                }
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("SteamSessionManager: Joined but connect string is empty!"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("SteamSessionManager: Failed to join session (Result: %d)"), static_cast<int32>(Result));
    }

    OnSteamSessionJoined.Broadcast(bSuccess, ConnectString);
}

void USteamSessionManager::ClientTravelToSteamHost(const FString& HostSteamId)
{
    if (UWorld* World = GetGameInstance()->GetWorld())
    {
        if (APlayerController* PC = World->GetFirstPlayerController())
        {
            FString Url = FString::Printf(TEXT("steam.%s"), *HostSteamId);
            UE_LOG(LogTemp, Warning, TEXT("SteamSessionManager: Direct P2P travel to %s"), *Url);
            PC->ClientTravel(Url, ETravelType::TRAVEL_Absolute);
            bIsInSession = true;
        }
    }
}

void USteamSessionManager::JoinSessionByInfo(const FMatchSessionInfo& SessionInfo)
{
    if (SessionInfo.IsSteamP2P())
    {
        if (!SessionInfo.HostSteamId.IsEmpty())
        {
            UE_LOG(LogTemp, Log, TEXT("SteamSessionManager: Joining P2P session - Host: %s (%s)"),
                *SessionInfo.HostPlayerName, *SessionInfo.HostSteamId);
            ClientTravelToSteamHost(SessionInfo.HostSteamId);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("SteamSessionManager: No HostSteamId for P2P session!"));
        }
    }
    else
    {
        if (UWorld* World = GetGameInstance()->GetWorld())
        {
            if (APlayerController* PC = World->GetFirstPlayerController())
            {
                FString TravelUrl = FString::Printf(TEXT("%s:%d"), *SessionInfo.ServerIp, SessionInfo.ServerPort);
                UE_LOG(LogTemp, Log, TEXT("SteamSessionManager: Joining dedicated server: %s"), *TravelUrl);
                PC->ClientTravel(TravelUrl, ETravelType::TRAVEL_Absolute);
            }
        }
    }
}

void USteamSessionManager::LeaveSteamSession()
{
    if (bIsHosting)
    {
        UnregisterSessionFromBackend();
        StopHeartbeatTimer();
    }

    IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get(STEAM_SUBSYSTEM);
    if (OnlineSub)
    {
        IOnlineSessionPtr SessionInterface = OnlineSub->GetSessionInterface();
        if (SessionInterface.IsValid())
        {
            DestroySessionDelegateHandle = SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(
                FOnDestroySessionCompleteDelegate::CreateUObject(this, &USteamSessionManager::OnDestroySessionComplete));
            SessionInterface->DestroySession(NAME_GameSession);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("SteamSessionManager: Leaving Steam session"));
}

void USteamSessionManager::OnDestroySessionComplete(FName InSessionName, bool bWasSuccessful)
{
    IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get(STEAM_SUBSYSTEM);
    if (OnlineSub)
    {
        IOnlineSessionPtr SessionInterface = OnlineSub->GetSessionInterface();
        if (SessionInterface.IsValid())
        {
            SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionDelegateHandle);
        }
    }

    bIsHosting = false;
    bIsInSession = false;
    HostedSession = FSteamHostedSession();

    UE_LOG(LogTemp, Log, TEXT("SteamSessionManager: Session destroyed (Success: %s)"),
        bWasSuccessful ? TEXT("true") : TEXT("false"));

    OnSteamSessionDestroyed.Broadcast(bWasSuccessful);

    if (!PendingSessionName.IsEmpty())
    {
        FString TempName = PendingSessionName;
        int32 TempMax = PendingMaxPlayers;
        FString TempMode = PendingGameMode;
        FString TempMap = PendingMapName;
        bool TempPrivate = PendingIsPrivate;

        CreateSteamSession(TempName, TempMax, TempMode, TempMap, TempPrivate);
    }
}

void USteamSessionManager::RegisterSessionWithBackend()
{
    UMatchmakingSubsystem* MatchSub = GetMatchmakingSubsystem();
    if (!MatchSub)
    {
        UE_LOG(LogTemp, Error, TEXT("SteamSessionManager: MatchmakingSubsystem not available!"));
        return;
    }

    if (!MatchSub->IsConnected())
    {
        UE_LOG(LogTemp, Warning, TEXT("SteamSessionManager: Not connected to matchmaking server, cannot register session"));
        return;
    }

    if (!MatchSub->OnSteamP2PRegistered.IsAlreadyBound(this, &USteamSessionManager::OnBackendRegistered))
    {
        MatchSub->OnSteamP2PRegistered.AddDynamic(this, &USteamSessionManager::OnBackendRegistered);
    }

    MatchSub->RegisterSteamP2PSession(
        HostedSession.SessionName,
        HostedSession.HostSteamId,
        HostedSession.HostPlayerName,
        HostedSession.GameMode,
        HostedSession.MaxPlayers,
        HostedSession.SteamLobbyId,
        HostedSession.bIsPrivate
    );

    UE_LOG(LogTemp, Log, TEXT("SteamSessionManager: Registering session '%s' (Map: %s, Mode: %s, Host: %s) with matchmaking server via TCP"),
        *HostedSession.SessionName, *HostedSession.MapName, *HostedSession.GameMode, *HostedSession.HostSteamId);
}

void USteamSessionManager::UnregisterSessionFromBackend()
{
    if (HostedSession.SessionId.IsEmpty())
    {
        return;
    }

    UMatchmakingSubsystem* MatchSub = GetMatchmakingSubsystem();
    if (MatchSub && MatchSub->IsConnected())
    {
        MatchSub->UnregisterSteamP2PSession(HostedSession.SessionId, HostedSession.HostSteamId);
        UE_LOG(LogTemp, Log, TEXT("SteamSessionManager: Unregistering session '%s' from matchmaking server"),
            *HostedSession.SessionId);
    }
}

void USteamSessionManager::OnBackendRegistered(bool bSuccess, const FString& SessionId)
{
    if (bSuccess)
    {
        HostedSession.SessionId = SessionId;
        UE_LOG(LogTemp, Log, TEXT("SteamSessionManager: Registered with matchmaking server. SessionId: %s"), *SessionId);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("SteamSessionManager: Failed to register with matchmaking server"));
    }
}

void USteamSessionManager::StartHeartbeatTimer()
{
    if (UWorld* World = GetGameInstance()->GetWorld())
    {
        World->GetTimerManager().SetTimer(
            HeartbeatTimerHandle,
            this,
            &USteamSessionManager::OnHeartbeatTick,
            HeartbeatInterval,
            true
        );
        UE_LOG(LogTemp, Log, TEXT("SteamSessionManager: Heartbeat timer started (%.0fs interval)"), HeartbeatInterval);
    }
}

void USteamSessionManager::StopHeartbeatTimer()
{
    if (UWorld* World = GetGameInstance()->GetWorld())
    {
        World->GetTimerManager().ClearTimer(HeartbeatTimerHandle);
    }
    HeartbeatTimerHandle.Invalidate();
}

void USteamSessionManager::OnHeartbeatTick()
{
    SendHeartbeat();
}

void USteamSessionManager::SendHeartbeat()
{
    if (HostedSession.SessionId.IsEmpty())
    {
        return;
    }

    UMatchmakingSubsystem* MatchSub = GetMatchmakingSubsystem();
    if (!MatchSub || !MatchSub->IsConnected())
    {
        return;
    }

    int32 CurrentPlayers = 1;
    FString CurrentMap = HostedSession.MapName;

    if (UWorld* World = GetGameInstance()->GetWorld())
    {
        CurrentPlayers = FMath::Max(1, World->GetNumPlayerControllers());
        FString WorldMap = World->GetMapName();
        if (!WorldMap.IsEmpty())
        {
            CurrentMap = WorldMap;
        }
    }

    HostedSession.CurrentPlayers = CurrentPlayers;
    MatchSub->SendSteamHeartbeat(HostedSession.SessionId, CurrentPlayers, CurrentMap);
}

FString USteamSessionManager::GetLocalSteamId() const
{
    IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get(STEAM_SUBSYSTEM);
    if (OnlineSub)
    {
        IOnlineIdentityPtr Identity = OnlineSub->GetIdentityInterface();
        if (Identity.IsValid())
        {
            TSharedPtr<const FUniqueNetId> UserId = Identity->GetUniquePlayerId(0);
            if (UserId.IsValid())
            {
                return UserId->ToString();
            }
        }
    }
    return TEXT("UnknownSteamId");
}

FString USteamSessionManager::GetLocalSteamName() const
{
    IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get(STEAM_SUBSYSTEM);
    if (OnlineSub)
    {
        IOnlineIdentityPtr Identity = OnlineSub->GetIdentityInterface();
        if (Identity.IsValid())
        {
            FString PlayerNickname = Identity->GetPlayerNickname(0);
            if (!PlayerNickname.IsEmpty())
            {
                return PlayerNickname;
            }
        }
    }
    return TEXT("UnknownPlayer");
}
