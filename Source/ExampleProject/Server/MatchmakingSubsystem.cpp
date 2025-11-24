// Fill out your copyright notice in the Description page of Project Settings.


#include "Server/MatchmakingSubsystem.h"
#include "TCPClientRunnable.h"
#include "Async/Async.h"
#include "Engine/Engine.h"
#include "MatchSessionInfo.h"

void UMatchmakingSubsystem::Initialize(FSubsystemCollectionBase& Collection) {
    Super::Initialize(Collection);
    ClientRunnable = nullptr;
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

void UMatchmakingSubsystem::HostNewGame(const FString& Name, int32 Port) {
    if (ClientRunnable) {
        ClientRunnable->HostNewGame(Name, Port);
    }
}

void UMatchmakingSubsystem::RefreshSessionList() {
    if (ClientRunnable && ClientRunnable->IsConnected()) {
        ClientRunnable->RequestSessionList();
        UE_LOG(LogTemp, Log, TEXT("RefreshSessionList - Requesting session list from server"));
    }

}

bool UMatchmakingSubsystem::IsConnected() const {
    return ClientRunnable ? ClientRunnable->IsConnected() : false;
}

void UMatchmakingSubsystem::HandleServerMessage(const FString& ServerMessage) {
    // This is always called on the GameThread by the runnable via AsyncTask
    if (ServerMessage.StartsWith(TEXT("s|"))) {
        ParseAndSetSessions(ServerMessage);
        OnSessionsUpdated.Broadcast(Sessions);
    }
    else if (ServerMessage.StartsWith(TEXT("o|"))) {
        // 'o|<port>|' indicates the server accepted host and gave us the port
        TArray<FString> Parts;
        ServerMessage.ParseIntoArray(Parts, TEXT("|"), true);
        if (Parts.Num() > 2) {
			FString IpAdress = *Parts[1];
            int32 Port = FCString::Atoi(*Parts[2]);
            OnHostRequested.Broadcast(IpAdress, Port);
        }
    }
}

void UMatchmakingSubsystem::HandleConnectionStatusChanged(bool bIsConnected) {
    UE_LOG(LogTemp, Warning, TEXT("MatchmakingSubsystem: Connection status changed - %s"), 
        bIsConnected ? TEXT("CONNECTED") : TEXT("DISCONNECTED"));
    OnConnectionStatusChanged.Broadcast(bIsConnected);
}

void UMatchmakingSubsystem::ParseAndSetSessions(const FString& ServerMessage) {
    // Example message format: s|<id>|<name>|<ip>|<port>|<id>|<name>|...|#
    TArray<FString> Out;
    ServerMessage.ParseIntoArray(Out, TEXT("|"), true);

    TArray<FMatchSessionInfo> NewSessions;
    // start at 1 because Out[0] == "s"
    for (int32 i = 1; i + 3 < Out.Num(); i += 4) {
        FMatchSessionInfo SI;
        SI.Id = FCString::Atoi(*Out[i]);
        SI.Name = Out[i + 1];
        SI.ServerIp = Out[i + 2];
        SI.ServerPort = FCString::Atoi(*Out[i + 3]);
        NewSessions.Add(SI);
    }

    Sessions = MoveTemp(NewSessions);
}

