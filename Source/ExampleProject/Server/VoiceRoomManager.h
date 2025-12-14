// VoiceRoomManager.h
// Server-side EOS RTC room credential generation for Trusted Server voice chat
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VoiceRoomManager.generated.h"

// Forward declaration for EOS callback (no conflict with SDK)

USTRUCT(BlueprintType)
struct FVoiceRoomCredentials
{
    GENERATED_BODY()

    /** The room name to join */
    UPROPERTY(BlueprintReadOnly)
    FString RoomName;

    /** The RTC token/credentials for joining */
    UPROPERTY(BlueprintReadOnly)
    FString ClientBaseUrl;

    /** Per-user token */
    UPROPERTY(BlueprintReadOnly)
    FString ParticipantToken;

    /** Is valid */
    UPROPERTY(BlueprintReadOnly)
    bool bIsValid = false;
};

// Delegate for when credentials are ready
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnVoiceCredentialsReady, const FString&, ProductUserId, FVoiceRoomCredentials, Credentials);

/**
 * Server-side manager for voice credentials (Trusted Server method).
 * 
 * NOTE: Requires server credentials with RTC Admin permissions from EOS Dev Portal.
 */
UCLASS(BlueprintType, Blueprintable)
class EXAMPLEPROJECT_API AVoiceRoomManager : public AActor
{
    GENERATED_BODY()

public:
    AVoiceRoomManager();

    /**
     * Request voice room credentials for a player.
     * @param ProductUserId - The client's EOS Product User ID
     * @param RoomName - The voice room to join (e.g., "Match_12345" or "Team_Blue")
     */
    UFUNCTION(BlueprintCallable, Category = "Voice|Server")
    void RequestVoiceCredentials(const FString& ProductUserId, const FString& RoomName);

    /**
     * Auto-generate credentials for main channel (convenience function)
     * Call this from GameMode when a player joins to auto-assign them to main voice channel
     * @param ProductUserId - The client's EOS Product User ID
     * @param MainChannelName - The main channel name (default: "MainChannel")
     */
    UFUNCTION(BlueprintCallable, Category = "Voice|Server")
    void AutoAssignMainChannel(const FString& ProductUserId, const FString& MainChannelName = TEXT("MainChannel"));

    /** Event fired when credentials are generated */
    UPROPERTY(BlueprintAssignable, Category = "Voice|Server")
    FOnVoiceCredentialsReady OnVoiceCredentialsReady;

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

private:
    void OnQueryJoinRoomTokenComplete(const FString& ProductUserId, const FString& RoomName, bool bSuccess, const FVoiceRoomCredentials& Credentials);
};
