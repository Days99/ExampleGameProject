// Fill out your copyright notice in the Description page of Project Settings.

#include "Server/MatchmakingSubsystem.h"
#include "TCPClientRunnable.h"
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
    if (ClientRunnable) return; // already running

    ClientRunnable = new FTCPClientRunnable(this);
    UE_LOG(LogTemp, Warning, TEXT("ConnectToMatchmakingServer"));
    // FTCPClientRunnable starts its own thread in the constructor
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

bool UMatchmakingSubsystem::IsConnected() const {
    return ClientRunnable ? ClientRunnable->IsConnected() : false;
}

void UMatchmakingSubsystem::HandleServerMessage(const FString& ServerMessage) {
    // This is always called on the GameThread by the runnable via AsyncTask

    // Session list response: s|id|name|ip|port|playercount|...|#
    if (ServerMessage.StartsWith(TEXT("s|"))) {
        ParseAndSetSessions(ServerMessage);
        OnSessionsUpdated.Broadcast(Sessions);
    }
    // Host confirmation response: o|sessionid|serverip|serverport|#
    else if (ServerMessage.StartsWith(TEXT("o|"))) {
        TArray<FString> Parts;
        ServerMessage.ParseIntoArray(Parts, TEXT("|"), true);

        //Server sends o|sessionid|serverip|serverport|
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
    // Join success response: j|success|serverip|serverport|#
    else if (ServerMessage.StartsWith(TEXT("j|"))) {
        TArray<FString> Parts;
        ServerMessage.ParseIntoArray(Parts, TEXT("|"), true);

        if (Parts.Num() >= 4 && Parts[1] == TEXT("success")) {
            FString ServerIp = Parts[2];
            int32 ServerPort = FCString::Atoi(*Parts[3]);

            UE_LOG(LogTemp, Log, TEXT("Join success - IP: %s, Port: %d"), *ServerIp, ServerPort);

            // Note: We don't know the session ID from this response, 
            // but we can track it from the join request if needed
            OnJoinSuccess.Broadcast(0, ServerIp, ServerPort);
        }
    }
    // Disconnect success response: d|success|#
    else if (ServerMessage.StartsWith(TEXT("d|"))) {
        TArray<FString> Parts;
        ServerMessage.ParseIntoArray(Parts, TEXT("|"), true);

        if (Parts.Num() >= 2 && Parts[1] == TEXT("success")) {
            CurrentSessionId = -1;
            UE_LOG(LogTemp, Log, TEXT("Disconnect success"));
            OnDisconnectSuccess.Broadcast();
        }
    }
    // Shutdown success response: k|success|#
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
    // Error response: e|errorcode|#
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
    // FIXED: Server format is now s|id|name|ip|port|playercount|...|#
    TArray<FString> Out;
    ServerMessage.ParseIntoArray(Out, TEXT("|"), true);

    TArray<FMatchSessionInfo> NewSessions;

    // Check if server sent "null" (no sessions)
    if (Out.Num() >= 2 && Out[1] == TEXT("null")) {
        UE_LOG(LogTemp, Log, TEXT("No sessions available"));
        Sessions = MoveTemp(NewSessions);
        return;
    }

    // start at 1 because Out[0] == "s"
    // Now expecting 5 fields per session: id, name, ip, port, playercount
    for (int32 i = 1; i + 4 < Out.Num(); i += 5) {
        // Skip if we hit the terminator
        if (Out[i] == TEXT("#")) break;

        FMatchSessionInfo SI;
        SI.Id = FCString::Atoi(*Out[i]);
        SI.Name = Out[i + 1];
        SI.ServerIp = Out[i + 2];
        SI.ServerPort = FCString::Atoi(*Out[i + 3]);
        SI.PlayerCount = FCString::Atoi(*Out[i + 4]); // ADDED

        NewSessions.Add(SI);

        UE_LOG(LogTemp, Log, TEXT("Parsed session: ID=%d, Name=%s, IP=%s, Port=%d, Players=%d"),
            SI.Id, *SI.Name, *SI.ServerIp, SI.ServerPort, SI.PlayerCount);
    }

    Sessions = MoveTemp(NewSessions);
}

