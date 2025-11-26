// Fill out your copyright notice in the Description page of Project Settings.

#include "Server/TCPClientRunnable.h"

#include "Sockets.h"
#include "SocketSubsystem.h"
#include "IPAddress.h"
#include "Server/MatchmakingSubsystem.h"
#include "Async/Async.h"
#include <Interfaces/IPv4/IPv4Address.h>

FTCPClientRunnable::FTCPClientRunnable(UMatchmakingSubsystem* InOwner)
    : Thread(nullptr)
    , Socket(nullptr)
    , bRun(true)
    , bConnected(false)
    , OwnerSubsystem(InOwner)
{
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
    // create socket
    Socket = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateSocket(NAME_Stream, TEXT("matchclient"), false);
    if (!Socket) return false;

    int32 NewSize = 0;
    Socket->SetReceiveBufferSize(1024, NewSize);

    FIPv4Address Addr(68, 221, 160, 33);
    TSharedRef<FInternetAddr> InternetAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
    InternetAddr->SetIp(Addr.Value);
    InternetAddr->SetPort(8856);

    bConnected = Socket->Connect(*InternetAddr);

    // Notify subsystem about connection status on game thread
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
    // After connection, send initial 'g|#' to request sessions
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
                // Ensure null-terminated
                Received[Read] = '\0';
                FString ServerMessage = FString(UTF8_TO_TCHAR(reinterpret_cast<const char*>(Received.GetData())));

                // marshal back to game thread
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

    // guard send with mutex to be safe if called from different threads
    FScopeLock Lock(&SendMutex);

    FTCHARToUTF8 Converter(*Message);
    int32 BytesSent = 0;
    Socket->Send((uint8*)Converter.Get(), Converter.Length(), BytesSent);
}

void FTCPClientRunnable::HostNewGame(const FString& Name) {
    if (!Socket) return;

    // Format: h|sessionname|#
    FString Serialized = FString::Printf(TEXT("h|%s|#"), *Name);
    SendRawString(Serialized);
}

void FTCPClientRunnable::RequestSessionList() {
    if (!Socket || !bConnected) return;
    SendRawString(TEXT("g|#"));
}

void FTCPClientRunnable::JoinSession(int32 SessionId) {
    if (!Socket || !bConnected) return;

    // Format: j|sessionid|#
    FString Message = FString::Printf(TEXT("j|%d|#"), SessionId);
    SendRawString(Message);
}

void FTCPClientRunnable::DisconnectFromSession() {
    if (!Socket || !bConnected) return;

    // Format: d|#
    SendRawString(TEXT("d|#"));
}

void FTCPClientRunnable::ShutdownSession(int32 SessionId) {
    if (!Socket || !bConnected) return;

    // Format: k|sessionid|#
    FString Message = FString::Printf(TEXT("k|%d|#"), SessionId);
    SendRawString(Message);
}

