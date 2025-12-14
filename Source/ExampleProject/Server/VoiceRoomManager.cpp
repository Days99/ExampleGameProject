// VoiceRoomManager.cpp
// Server-side EOS RTC credential generation for Trusted Server voice chat
// 
// NOTE: Full implementation requires EOS SDK integration
// The EOS SDK must be initialized on the server with SERVER credentials
// (different from client credentials - requires "Server" client type in EOS Dev Portal)

#include "VoiceRoomManager.h"

AVoiceRoomManager::AVoiceRoomManager()
{
    PrimaryActorTick.bCanEverTick = true; // Need tick to process EOS callbacks
}

void AVoiceRoomManager::BeginPlay()
{
    Super::BeginPlay();
    
    if (HasAuthority())
    {
        UE_LOG(LogTemp, Log, TEXT("[VoiceRoomManager] Ready on server (Trusted Server voice requires RTC Admin credentials)"));
    }
}

void AVoiceRoomManager::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}


void AVoiceRoomManager::AutoAssignMainChannel(const FString& ProductUserId, const FString& MainChannelName)
{
    UE_LOG(LogTemp, Log, TEXT("[VoiceRoomManager] Auto-assigning player %s to main channel: %s"), *ProductUserId, *MainChannelName);
    RequestVoiceCredentials(ProductUserId, MainChannelName);
}

void AVoiceRoomManager::RequestVoiceCredentials(const FString& ProductUserId, const FString& RoomName)
{
    UE_LOG(LogTemp, Log, TEXT("[VoiceRoomManager] Generating credentials for user %s, room %s"), *ProductUserId, *RoomName);
    
    // EOS Voice Chat expects credentials in JSON format
    // Format: {"roomId":"...", "clientBaseUrl":"...", "participantToken":"..."}
    FString JsonCredentials = FString::Printf(TEXT(
        "{"
        "\"roomId\":\"%s\","
        "\"clientBaseUrl\":\"wss://rtc-gll-a-prod.ol.epicgames.net\","
        "\"participantToken\":\"%s\""
        "}"
    ), *RoomName, *ProductUserId);
    
    FVoiceRoomCredentials Credentials;
    Credentials.RoomName = RoomName;
    Credentials.ClientBaseUrl = TEXT("wss://rtc-gll-a-prod.ol.epicgames.net");
    Credentials.ParticipantToken = JsonCredentials; // Full JSON as token
    Credentials.bIsValid = true;
    
    UE_LOG(LogTemp, Log, TEXT("[VoiceRoomManager] Credentials generated: %s"), *JsonCredentials);
    OnQueryJoinRoomTokenComplete(ProductUserId, RoomName, true, Credentials);
}

void AVoiceRoomManager::OnQueryJoinRoomTokenComplete(const FString& ProductUserId, const FString& RoomName, bool bSuccess, const FVoiceRoomCredentials& Credentials)
{
    if (bSuccess)
    {
        UE_LOG(LogTemp, Log, TEXT("[VoiceRoomManager] ================================================"));
        UE_LOG(LogTemp, Log, TEXT("[VoiceRoomManager] Voice credentials generated successfully"));
        UE_LOG(LogTemp, Log, TEXT("[VoiceRoomManager] ProductUserId: %s"), *ProductUserId);
        UE_LOG(LogTemp, Log, TEXT("[VoiceRoomManager] RoomName: %s"), *RoomName);
        UE_LOG(LogTemp, Log, TEXT("[VoiceRoomManager] ClientBaseUrl: %s"), *Credentials.ClientBaseUrl);
        UE_LOG(LogTemp, Log, TEXT("[VoiceRoomManager] ParticipantToken: %s"), *Credentials.ParticipantToken);
        UE_LOG(LogTemp, Log, TEXT("[VoiceRoomManager] ================================================"));
        UE_LOG(LogTemp, Log, TEXT("[VoiceRoomManager] Send these credentials to client via RPC"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[VoiceRoomManager] Failed to generate credentials for %s"), *ProductUserId);
    }

    // Broadcast to whoever needs to send this to the client
    OnVoiceCredentialsReady.Broadcast(ProductUserId, Credentials);
}

/*
================================================================================
INTEGRATION NOTES
================================================================================

To fully implement this, you need:

1. EOS SDK Integration:
   - Download EOS SDK from Epic Games Developer Portal
   - Link EOS SDK libraries to your server build
   - Include EOS SDK headers

2. Server Credentials Setup:
   - In EOS Dev Portal, create a "Server" client (not "Game Client")
   - Server client has different ClientId/ClientSecret than game clients
   - Server client must have RTC Admin permissions enabled

3. Alternative: Use EOSIntegrationKit Plugin
   - The EOSIntegrationKit plugin already has this implemented
   - Consider using their VoiceRoomManager as reference
   - https://github.com/betidestudio/EOSIntegrationKit

4. Client-Server Flow:
   - Client: VoiceChatComponent->GetProductUserId()
   - Client: Send ProductUserId to server via RPC
   - Server: VoiceRoomManager->RequestVoiceCredentials(ProductUserId, RoomName)
   - Server: Receive credentials via OnVoiceCredentialsReady delegate
   - Server: Send credentials to client via RPC
   - Client: VoiceChatComponent->JoinChannel(RoomName, Token)

================================================================================
*/
