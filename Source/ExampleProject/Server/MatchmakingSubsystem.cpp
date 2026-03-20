// Fill out your copyright notice in the Description page of Project Settings.

#include "Server/MatchmakingSubsystem.h"
#include "TCPClientRunnable.h"
#include "BackendSettings.h"
#include "SteamSessionManager.h"
#include "Async/Async.h"
#include "Engine/Engine.h"
#include "MatchSessionInfo.h"

void UMatchmakingSubsystem::Initialize(FSubsystemCollectionBase& Collection) {
    Super::Initialize(Collection);
    ClientRunnable = nullptr;
    CurrentSessionId = -1;
}

void UMatchmakingSubsystem::Deinitialize() {
    if (ClientRunnable) {
        ClientRunnable->Stop();
        delete ClientRunnable;
        ClientRunnable = nullptr;
    }
    Super::Deinitialize();
}

void UMatchmakingSubsystem::ConnectToMatchmakingServer() {
    if (ClientRunnable) return;

    FString BackendIP = TEXT("127.0.0.1");
    int32 BackendPort = 8856;

    if (UGameInstance* GI = GetGameInstance()) {
        if (UBackendSettings* Settings = GI->GetSubsystem<UBackendSettings>()) {
            BackendIP = Settings->GetBackendIP();
            BackendPort = Settings->GetBackendPort();
        }
    }

    ClientRunnable = new FTCPClientRunnable(this, BackendIP, BackendPort);
    UE_LOG(LogTemp, Warning, TEXT("ConnectToMatchmakingServer: %s:%d"), *BackendIP, BackendPort);
}

void UMatchmakingSubsystem::HostNewGame(const FString& Name) {
    if (ClientRunnable) {
        ClientRunnable->HostNewGame(Name);
    }
}

void UMatchmakingSubsystem::RefreshSessionList() {
    if (ClientRunnable && ClientRunnable->IsConnected()) {
        ClientRunnable->RequestSessionList();
        UE_LOG(LogTemp, Log, TEXT("RefreshSessionList - Requesting session list from server"));
    }
}

void UMatchmakingSubsystem::JoinSession(int32 SessionId) {
    if (ClientRunnable && ClientRunnable->IsConnected()) {
        ClientRunnable->JoinSession(SessionId);
        UE_LOG(LogTemp, Log, TEXT("JoinSession - Requesting to join session %d"), SessionId);
    }
}

void UMatchmakingSubsystem::DisconnectFromSession() {
    if (ClientRunnable && ClientRunnable->IsConnected()) {
        ClientRunnable->DisconnectFromSession();
        UE_LOG(LogTemp, Log, TEXT("DisconnectFromSession - Requesting to leave current session"));
    }
}

void UMatchmakingSubsystem::ShutdownSession(int32 SessionId) {
    if (ClientRunnable && ClientRunnable->IsConnected()) {
        ClientRunnable->ShutdownSession(SessionId);
        UE_LOG(LogTemp, Log, TEXT("ShutdownSession - Requesting to shutdown session %d"), SessionId);
    }
}

// --- Steam P2P Commands (forwarded to the same TCP matchmaking server) ---

void UMatchmakingSubsystem::RegisterSteamP2PSession(const FString& SessionName, const FString& HostSteamId,
    const FString& HostPlayerName, const FString& GameMode, int32 MaxPlayers,
    const FString& SteamLobbyId, bool bIsPrivate)
{
    if (ClientRunnable && ClientRunnable->IsConnected()) {
        ClientRunnable->RegisterSteamP2PSession(SessionName, HostSteamId, HostPlayerName,
            GameMode, MaxPlayers, SteamLobbyId, bIsPrivate);
        UE_LOG(LogTemp, Log, TEXT("RegisterSteamP2PSession - Sending to matchmaking server"));
    }
    else {
        UE_LOG(LogTemp, Warning, TEXT("RegisterSteamP2PSession - Not connected to matchmaking server!"));
        OnSteamP2PRegistered.Broadcast(false, TEXT(""));
    }
}

void UMatchmakingSubsystem::SendSteamHeartbeat(const FString& SessionId, int32 CurrentPlayers, const FString& MapName)
{
    if (ClientRunnable && ClientRunnable->IsConnected()) {
        ClientRunnable->SendSteamHeartbeat(SessionId, CurrentPlayers, MapName);
    }
}

void UMatchmakingSubsystem::UnregisterSteamP2PSession(const FString& SessionId, const FString& HostSteamId)
{
    if (ClientRunnable && ClientRunnable->IsConnected()) {
        ClientRunnable->UnregisterSteamP2PSession(SessionId, HostSteamId);
        UE_LOG(LogTemp, Log, TEXT("UnregisterSteamP2PSession - Sending to matchmaking server"));
    }
    else {
        UE_LOG(LogTemp, Warning, TEXT("UnregisterSteamP2PSession - Not connected to matchmaking server!"));
        OnSteamP2PUnregistered.Broadcast(false);
    }
}

bool UMatchmakingSubsystem::IsConnected() const {
    return ClientRunnable ? ClientRunnable->IsConnected() : false;
}

void UMatchmakingSubsystem::HandleServerMessage(const FString& ServerMessage) {
    // Session list response
    if (ServerMessage.StartsWith(TEXT("s|"))) {
        ParseAndSetSessions(ServerMessage);
        OnSessionsUpdated.Broadcast(Sessions);
    }
    // Host confirmation: o|sessionid|serverip|serverport|#
    else if (ServerMessage.StartsWith(TEXT("o|"))) {
        TArray<FString> Parts;
        ServerMessage.ParseIntoArray(Parts, TEXT("|"), true);

        if (Parts.Num() >= 4) {
            int32 SessionId = FCString::Atoi(*Parts[1]);
            FString ServerIp = Parts[2];
            int32 ServerPort = FCString::Atoi(*Parts[3]);
            CurrentSessionId = SessionId;

            UE_LOG(LogTemp, Log, TEXT("Host confirmed - SessionId: %d, IP: %s, Port: %d"),
                SessionId, *ServerIp, ServerPort);
            OnHostRequested.Broadcast(SessionId, ServerIp, ServerPort);
        }
    }
    // Join success: j|success|serverip|serverport|#
    else if (ServerMessage.StartsWith(TEXT("j|"))) {
        TArray<FString> Parts;
        ServerMessage.ParseIntoArray(Parts, TEXT("|"), true);

        if (Parts.Num() >= 4 && Parts[1] == TEXT("success")) {
            FString ServerIp = Parts[2];
            int32 ServerPort = FCString::Atoi(*Parts[3]);
            UE_LOG(LogTemp, Log, TEXT("Join success - IP: %s, Port: %d"), *ServerIp, ServerPort);
            OnJoinSuccess.Broadcast(0, ServerIp, ServerPort);
        }
    }
    // Disconnect success: d|success|#
    else if (ServerMessage.StartsWith(TEXT("d|"))) {
        TArray<FString> Parts;
        ServerMessage.ParseIntoArray(Parts, TEXT("|"), true);

        if (Parts.Num() >= 2 && Parts[1] == TEXT("success")) {
            CurrentSessionId = -1;
            UE_LOG(LogTemp, Log, TEXT("Disconnect success"));
            OnDisconnectSuccess.Broadcast();
        }
    }
    // Shutdown success: k|success|#
    else if (ServerMessage.StartsWith(TEXT("k|"))) {
        TArray<FString> Parts;
        ServerMessage.ParseIntoArray(Parts, TEXT("|"), true);

        if (Parts.Num() >= 2 && Parts[1] == TEXT("success")) {
            int32 ShutdownSessionId = CurrentSessionId;
            CurrentSessionId = -1;
            UE_LOG(LogTemp, Log, TEXT("Shutdown success for session %d"), ShutdownSessionId);
            OnShutdownSuccess.Broadcast(ShutdownSessionId);
        }
    }
    // Steam P2P register response: p|success|sessionid|# or p|error|reason|#
    else if (ServerMessage.StartsWith(TEXT("p|"))) {
        TArray<FString> Parts;
        ServerMessage.ParseIntoArray(Parts, TEXT("|"), true);

        if (Parts.Num() >= 3 && Parts[1] == TEXT("success")) {
            FString SessionId = Parts[2];
            UE_LOG(LogTemp, Log, TEXT("Steam P2P registered - SessionId: %s"), *SessionId);
            OnSteamP2PRegistered.Broadcast(true, SessionId);
        }
        else {
            FString Reason = (Parts.Num() >= 3) ? Parts[2] : TEXT("unknown");
            UE_LOG(LogTemp, Warning, TEXT("Steam P2P register failed: %s"), *Reason);
            OnSteamP2PRegistered.Broadcast(false, TEXT(""));
        }
    }
    // Steam P2P unregister response: u|success|#
    else if (ServerMessage.StartsWith(TEXT("u|"))) {
        TArray<FString> Parts;
        ServerMessage.ParseIntoArray(Parts, TEXT("|"), true);

        bool bSuccess = (Parts.Num() >= 2 && Parts[1] == TEXT("success"));
        if (bSuccess) {
            UE_LOG(LogTemp, Log, TEXT("Steam P2P unregistered successfully"));
        }
        else {
            UE_LOG(LogTemp, Warning, TEXT("Steam P2P unregister failed"));
        }
        OnSteamP2PUnregistered.Broadcast(bSuccess);
    }
    // Steam heartbeat ack: b|ok|#
    else if (ServerMessage.StartsWith(TEXT("b|"))) {
        UE_LOG(LogTemp, Verbose, TEXT("Steam P2P heartbeat acknowledged"));
    }
    // Error: e|errorcode|#
    else if (ServerMessage.StartsWith(TEXT("e|"))) {
        TArray<FString> Parts;
        ServerMessage.ParseIntoArray(Parts, TEXT("|"), true);

        if (Parts.Num() >= 2) {
            FString ErrorCode = Parts[1];
            UE_LOG(LogTemp, Error, TEXT("Server error: %s"), *ErrorCode);
            OnServerError.Broadcast(ErrorCode);
        }
    }
    else {
        UE_LOG(LogTemp, Warning, TEXT("Unknown server message: %s"), *ServerMessage);
    }
}

void UMatchmakingSubsystem::HandleConnectionStatusChanged(bool bIsConnected) {
    UE_LOG(LogTemp, Warning, TEXT("MatchmakingSubsystem: Connection status changed - %s"),
        bIsConnected ? TEXT("CONNECTED") : TEXT("DISCONNECTED"));
    OnConnectionStatusChanged.Broadcast(bIsConnected);
}

void UMatchmakingSubsystem::ParseAndSetSessions(const FString& ServerMessage) {
    TArray<FString> Out;
    ServerMessage.ParseIntoArray(Out, TEXT("|"), true);

    TArray<FMatchSessionInfo> NewSessions;

    if (Out.Num() >= 2 && Out[1] == TEXT("null")) {
        UE_LOG(LogTemp, Log, TEXT("No sessions available"));
        Sessions = MoveTemp(NewSessions);
        return;
    }

    // Supported formats per session:
    // Dedicated (legacy/current): id, name, ip, port, playercount
    // Steam P2P (current backend):
    //   sessionid, name, hoststeamid, steamlobbyid, isprivate(0/1), currentplayers, steam_p2p
    // Extended mixed format (legacy): id, name, ip, port, playercount, sessiontype, hoststeamid, hostplayername, maxplayers
    int32 i = 1;
    while (i < Out.Num()) {
        if (Out[i] == TEXT("#")) break;
        if (i + 1 >= Out.Num()) break;

        FMatchSessionInfo SI;
        SI.SessionId = Out[i];
        SI.Name = Out[i + 1];

        // Steam compact format check first:
        // sessionid|name|hoststeamid|steamlobbyid|isprivate|currentplayers|steam_p2p
        if (i + 6 < Out.Num() && Out[i + 6] == TEXT("steam_p2p"))
        {
            SI.SessionType = TEXT("steam_p2p");
            SI.HostSteamId = Out[i + 2];
            SI.SteamLobbyId = Out[i + 3];
            SI.bIsPrivate = (FCString::Atoi(*Out[i + 4]) != 0);
            SI.PlayerCount = FCString::Atoi(*Out[i + 5]);
            SI.MaxPlayers = 8; // Not included in list payload

            // Keep an integer identifier for existing UI button/delegate flow.
            SI.Id = static_cast<int32>(GetTypeHash(SI.SessionId) & 0x7FFFFFFF);

            i += 7;

            UE_LOG(LogTemp, Log, TEXT("Parsed session: SessionId=%s, Name=%s, Type=%s, Players=%d/%d, HostSteamId=%s"),
                *SI.SessionId, *SI.Name, *SI.SessionType, SI.PlayerCount, SI.MaxPlayers, *SI.HostSteamId);
        }
        // Extended format check
        else if (i + 8 < Out.Num() && Out[i + 5] != TEXT("#") &&
            (Out[i + 5] == TEXT("dedicated") || Out[i + 5] == TEXT("steam_p2p")))
        {
            SI.Id = FCString::Atoi(*Out[i]);
            SI.SessionType = Out[i + 5];
            SI.HostSteamId = Out[i + 6];
            SI.HostPlayerName = Out[i + 7];
            SI.MaxPlayers = FCString::Atoi(*Out[i + 8]);
            SI.ServerIp = Out[i + 2];
            SI.ServerPort = FCString::Atoi(*Out[i + 3]);
            SI.PlayerCount = FCString::Atoi(*Out[i + 4]);
            i += 9;

            UE_LOG(LogTemp, Log, TEXT("Parsed session: SessionId=%s, ID=%d, Name=%s, Type=%s, Players=%d/%d, Host=%s (%s)"),
                *SI.SessionId, SI.Id, *SI.Name, *SI.SessionType, SI.PlayerCount, SI.MaxPlayers, *SI.HostPlayerName, *SI.HostSteamId);
        }
        else
        {
            // Dedicated fallback:
            // id, name, ip, port, playercount
            if (i + 4 >= Out.Num()) break;
            SI.Id = FCString::Atoi(*Out[i]);
            SI.ServerIp = Out[i + 2];
            SI.ServerPort = FCString::Atoi(*Out[i + 3]);
            SI.PlayerCount = FCString::Atoi(*Out[i + 4]);
            SI.SessionType = TEXT("dedicated");
            i += 5;

            UE_LOG(LogTemp, Log, TEXT("Parsed session: SessionId=%s, ID=%d, Name=%s, IP=%s, Port=%d, Players=%d"),
                *SI.SessionId, SI.Id, *SI.Name, *SI.ServerIp, SI.ServerPort, SI.PlayerCount);
        }

        NewSessions.Add(SI);
    }

    Sessions = MoveTemp(NewSessions);
}

USteamSessionManager* UMatchmakingSubsystem::GetSteamSessionManager() const
{
    if (UGameInstance* GI = GetGameInstance())
    {
        return GI->GetSubsystem<USteamSessionManager>();
    }
    return nullptr;
}

void UMatchmakingSubsystem::JoinSessionByInfo(const FMatchSessionInfo& SessionInfo)
{
    if (SessionInfo.IsSteamP2P())
    {
        USteamSessionManager* SteamMgr = GetSteamSessionManager();
        if (SteamMgr)
        {
            SteamMgr->JoinSessionByInfo(SessionInfo);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("MatchmakingSubsystem: SteamSessionManager not available for P2P join!"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("MatchmakingSubsystem: Joining dedicated server %s:%d"), *SessionInfo.ServerIp, SessionInfo.ServerPort);
        if (ClientRunnable && ClientRunnable->IsConnected())
        {
            ClientRunnable->JoinSession(SessionInfo.Id);
        }
    }
}
