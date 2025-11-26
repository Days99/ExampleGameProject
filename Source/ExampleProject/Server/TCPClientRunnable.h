// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HAL/Runnable.h"
#include "Templates/SharedPointer.h"


class FSocket;
class UMatchmakingSubsystem;

/**
 * TCP Client Runnable for connecting to matchmaking server
 */
class FTCPClientRunnable : public FRunnable
{
public:
    FTCPClientRunnable(UMatchmakingSubsystem* InOwner);
    virtual ~FTCPClientRunnable();

    // FRunnable interface
    virtual bool Init() override;
    virtual uint32 Run() override;
    virtual void Stop() override;

    // Connection status
    bool IsConnected() const { return bConnected; }

    // Send commands to server
    void HostNewGame(const FString& Name);
    void RequestSessionList();
    void JoinSession(int32 SessionId);
    void DisconnectFromSession();
    void ShutdownSession(int32 SessionId);

private:
    void SendRawString(const FString& Message);

    FRunnableThread* Thread;
    FSocket* Socket;
    FThreadSafeBool bRun;
    FThreadSafeBool bConnected;
    TWeakObjectPtr<UMatchmakingSubsystem> OwnerSubsystem;
    FCriticalSection SendMutex;
};

