#pragma once

#include "CoreMinimal.h"
#include "MatchSessionInfo.generated.h"

USTRUCT(BlueprintType)
struct FMatchSessionInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Matchmaking")
    int32 Id = 0;

    /** Backend-assigned session identifier (GUID/string for steam_p2p) */
    UPROPERTY(BlueprintReadOnly, Category = "Matchmaking")
    FString SessionId;

    UPROPERTY(BlueprintReadOnly, Category = "Matchmaking")
    FString Name;

    UPROPERTY(BlueprintReadOnly, Category = "Matchmaking")
    FString ServerIp;

    UPROPERTY(BlueprintReadOnly, Category = "Matchmaking")
    int32 ServerPort = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Matchmaking")
    int32 PlayerCount = 0;

    /** Session type: "dedicated" or "steam_p2p" */
    UPROPERTY(BlueprintReadOnly, Category = "Matchmaking|Steam")
    FString SessionType = TEXT("dedicated");

    /** Host's Steam ID (only for steam_p2p sessions) */
    UPROPERTY(BlueprintReadOnly, Category = "Matchmaking|Steam")
    FString HostSteamId;

    /** Host's display name (only for steam_p2p sessions) */
    UPROPERTY(BlueprintReadOnly, Category = "Matchmaking|Steam")
    FString HostPlayerName;

    /** Steam lobby ID for joining (only for steam_p2p sessions) */
    UPROPERTY(BlueprintReadOnly, Category = "Matchmaking|Steam")
    FString SteamLobbyId;

    /** Game mode tag */
    UPROPERTY(BlueprintReadOnly, Category = "Matchmaking")
    FString GameMode;

    /** Map name */
    UPROPERTY(BlueprintReadOnly, Category = "Matchmaking")
    FString MapName;

    /** Max players allowed */
    UPROPERTY(BlueprintReadOnly, Category = "Matchmaking")
    int32 MaxPlayers = 8;

    /** Whether this is a private session */
    UPROPERTY(BlueprintReadOnly, Category = "Matchmaking")
    bool bIsPrivate = false;

    /** Check if this session is a Steam P2P session */
    bool IsSteamP2P() const { return SessionType == TEXT("steam_p2p"); }

    /** Check if this session has available slots */
    bool HasAvailableSlots() const { return PlayerCount < MaxPlayers; }
};

/** Struct specifically for Steam hosted session data (used when creating/registering) */
USTRUCT(BlueprintType)
struct FSteamHostedSession
{
    GENERATED_BODY()

    /** Backend-assigned session ID */
    UPROPERTY(BlueprintReadWrite, Category = "Steam")
    FString SessionId;

    /** Display name for the session */
    UPROPERTY(BlueprintReadWrite, Category = "Steam")
    FString SessionName;

    /** Host's Steam ID */
    UPROPERTY(BlueprintReadWrite, Category = "Steam")
    FString HostSteamId;

    /** Host's display name */
    UPROPERTY(BlueprintReadWrite, Category = "Steam")
    FString HostPlayerName;

    /** Game mode */
    UPROPERTY(BlueprintReadWrite, Category = "Steam")
    FString GameMode;

    /** Map name */
    UPROPERTY(BlueprintReadWrite, Category = "Steam")
    FString MapName;    /** Current player count */
    UPROPERTY(BlueprintReadWrite, Category = "Steam")
    int32 CurrentPlayers = 1;

    /** Maximum players allowed */
    UPROPERTY(BlueprintReadWrite, Category = "Steam")
    int32 MaxPlayers = 8;

    /** Is this a private session */
    UPROPERTY(BlueprintReadWrite, Category = "Steam")
    bool bIsPrivate = false;

    /** Steam lobby ID */
    UPROPERTY(BlueprintReadWrite, Category = "Steam")
    FString SteamLobbyId;
};
