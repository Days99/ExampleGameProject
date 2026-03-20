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
    FTCPClientRunnable(UMatchmakingSubsystem* InOwner, const FString& InServerIP = TEXT("127.0.0.1"), int32 InServerPort = 8856);
    virtual ~FTCPClientRunnable();

    virtual bool Init() override;
    virtual uint32 Run() override;
    virtual void Stop() override;

    bool IsConnected() const { return bConnected; }

    void HostNewGame(const FString& Name);
    void RequestSessionList();
    void JoinSession(int32 SessionId);
    void DisconnectFromSession();
    void ShutdownSession(int32 SessionId);

    /** Register a Steam P2P session with the matchmaking server
     *  Format: p|sessionname|hoststeamid|hostplayername|gamemode|maxplayers|steamlobbyid|isprivate|# */
    void RegisterSteamP2PSession(const FString& SessionName, const FString& HostSteamId,
        const FString& HostPlayerName, const FString& GameMode, int32 MaxPlayers,
        const FString& SteamLobbyId, bool bIsPrivate);

    /** Send heartbeat for an active Steam P2P session
     *  Format: b|sessionid|currentplayers|mapname|# */
    void SendSteamHeartbeat(const FString& SessionId, int32 CurrentPlayers, const FString& MapName);

    /** Unregister a Steam P2P session from the matchmaking server
     *  Format: u|sessionid|hoststeamid|# */
    void UnregisterSteamP2PSession(const FString& SessionId, const FString& HostSteamId);

private:
    void SendRawString(const FString& Message);

    FRunnableThread* Thread;
    FSocket* Socket;
    FThreadSafeBool bRun;
    FThreadSafeBool bConnected;
    TWeakObjectPtr<UMatchmakingSubsystem> OwnerSubsystem;
    FCriticalSection SendMutex;

    FString ServerIP;
    int32 ServerPort;
};
