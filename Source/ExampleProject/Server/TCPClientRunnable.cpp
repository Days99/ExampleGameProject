#include "Server/TCPClientRunnable.h"

#include "Sockets.h"
#include "SocketSubsystem.h"
#include "IPAddress.h"
#include "Server/MatchmakingSubsystem.h"
#include "Async/Async.h"
#include <Interfaces/IPv4/IPv4Address.h>

FTCPClientRunnable::FTCPClientRunnable(UMatchmakingSubsystem* InOwner, const FString& InServerIP, int32 InServerPort)
    : Thread(nullptr)
    , Socket(nullptr)
    , bRun(true)
    , bConnected(false)
    , OwnerSubsystem(InOwner)
    , ServerIP(InServerIP)
    , ServerPort(InServerPort)
{
    UE_LOG(LogTemp, Log, TEXT("TCPClientRunnable: Connecting to %s:%d"), *ServerIP, ServerPort);
    Thread = FRunnableThread::Create(this, TEXT("FTCPClientRunnable"), 0, TPri_Normal);
}

FTCPClientRunnable::~FTCPClientRunnable() {
    Stop();
    if (Thread) {
        Thread->Kill(true);
        delete Thread;
        Thread = nullptr;
    }
    if (Socket) {
        Socket->Close();
        ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(Socket);
        Socket = nullptr;
    }
}

bool FTCPClientRunnable::Init() {
    Socket = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateSocket(NAME_Stream, TEXT("matchclient"), false);
    if (!Socket) return false;

    int32 NewSize = 0;
    Socket->SetReceiveBufferSize(1024, NewSize);

    FIPv4Address Addr;
    if (!FIPv4Address::Parse(ServerIP, Addr))
    {
        UE_LOG(LogTemp, Error, TEXT("TCPClientRunnable: Failed to parse IP address: %s"), *ServerIP);
        return false;
    }
    TSharedRef<FInternetAddr> InternetAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
    InternetAddr->SetIp(Addr.Value);
    InternetAddr->SetPort(ServerPort);

    UE_LOG(LogTemp, Log, TEXT("TCPClientRunnable: Attempting connection to %s:%d"), *ServerIP, ServerPort);

    bConnected = Socket->Connect(*InternetAddr);

    if (OwnerSubsystem.IsValid()) {
        TWeakObjectPtr<UMatchmakingSubsystem> WeakOwner = OwnerSubsystem;
        bool bConnectionSuccess = bConnected;
        AsyncTask(ENamedThreads::GameThread, [WeakOwner, bConnectionSuccess]() {
            if (WeakOwner.IsValid()) {
                WeakOwner->HandleConnectionStatusChanged(bConnectionSuccess);
            }
        });
    }

    return true;
}

uint32 FTCPClientRunnable::Run() {
    if (bConnected) {
        SendRawString(TEXT("g|#"));
    }

    while (bRun) {
        if (!Socket) { FPlatformProcess::Sleep(0.1f); continue; }

        uint32 Pending = 0;
        if (Socket->HasPendingData(Pending) && Pending > 0) {
            TArray<uint8> Received;
            Received.SetNumUninitialized(Pending + 1);
            int32 Read = 0;
            if (Socket->Recv(Received.GetData(), Pending, Read)) {
                Received[Read] = '\0';
                FString ServerMessage = FString(UTF8_TO_TCHAR(reinterpret_cast<const char*>(Received.GetData())));

                if (OwnerSubsystem.IsValid()) {
                    TWeakObjectPtr<UMatchmakingSubsystem> WeakOwner = OwnerSubsystem;
                    AsyncTask(ENamedThreads::GameThread, [WeakOwner, ServerMessage]() {
                        if (WeakOwner.IsValid()) {
                            WeakOwner->HandleServerMessage(ServerMessage);
                        }
                    });
                }
            }
        }
        else {
            FPlatformProcess::Sleep(0.05f);
        }
    }
    return 0;
}

void FTCPClientRunnable::Stop() {
    bRun = false;
}

void FTCPClientRunnable::SendRawString(const FString& Message) {
    if (!Socket) return;

    FScopeLock Lock(&SendMutex);

    FTCHARToUTF8 Converter(*Message);
    int32 BytesSent = 0;
    Socket->Send((uint8*)Converter.Get(), Converter.Length(), BytesSent);
}

void FTCPClientRunnable::HostNewGame(const FString& Name) {
    if (!Socket) return;
    FString Serialized = FString::Printf(TEXT("h|%s|#"), *Name);
    SendRawString(Serialized);
}

void FTCPClientRunnable::RequestSessionList() {
    if (!Socket || !bConnected) return;
    SendRawString(TEXT("g|#"));
}

void FTCPClientRunnable::JoinSession(int32 SessionId) {
    if (!Socket || !bConnected) return;
    FString Message = FString::Printf(TEXT("j|%d|#"), SessionId);
    SendRawString(Message);
}

void FTCPClientRunnable::DisconnectFromSession() {
    if (!Socket || !bConnected) return;
    SendRawString(TEXT("d|#"));
}

void FTCPClientRunnable::ShutdownSession(int32 SessionId) {
    if (!Socket || !bConnected) return;
    FString Message = FString::Printf(TEXT("k|%d|#"), SessionId);
    SendRawString(Message);
}

void FTCPClientRunnable::RegisterSteamP2PSession(
    const FString& SessionName,
    const FString& HostSteamId,
    const FString& HostPlayerName,
    const FString& GameMode,
    int32 MaxPlayers,
    const FString& SteamLobbyId,
    bool bIsPrivate)
{
    if (!Socket || !bConnected) return;

    FString Message = FString::Printf(TEXT("p|%s|%s|%s|%s|%d|%s|%d|#"),
        *SessionName, *HostSteamId, *HostPlayerName, *GameMode, MaxPlayers,
        *SteamLobbyId, bIsPrivate ? 1 : 0);
    SendRawString(Message);

    UE_LOG(LogTemp, Log, TEXT("TCPClientRunnable: Sent Steam P2P register - %s"), *Message);
}

void FTCPClientRunnable::SendSteamHeartbeat(
    const FString& SessionId,
    int32 CurrentPlayers,
    const FString& MapName)
{
    if (!Socket || !bConnected) return;

    FString Message = FString::Printf(TEXT("b|%s|%d|%s|#"),
        *SessionId, CurrentPlayers, *MapName);
    SendRawString(Message);
}

void FTCPClientRunnable::UnregisterSteamP2PSession(
    const FString& SessionId,
    const FString& HostSteamId)
{
    if (!Socket || !bConnected) return;

    FString Message = FString::Printf(TEXT("u|%s|%s|#"),
        *SessionId, *HostSteamId);
    SendRawString(Message);

    UE_LOG(LogTemp, Log, TEXT("TCPClientRunnable: Sent Steam P2P unregister - SessionId: %s"), *SessionId);
}
