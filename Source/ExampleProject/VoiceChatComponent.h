// VoiceChatComponent.h
// Simplified EOS Voice Chat Component for Trusted Server Method
// Based on EOSIntegrationKit pattern with latest EOS API
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "OnlineSubsystem.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "VoiceChat.h"
#include "VoiceChatComponent.generated.h"

// Delegate when voice system is ready
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnVoiceChatReady);

// Delegate when joining a voice channel completes
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnVoiceChannelJoined, const FString&, ChannelName, bool, bSuccess);

// Delegate when leaving a voice channel completes
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnVoiceChannelLeft, const FString&, ChannelName, bool, bSuccess);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class EXAMPLEPROJECT_API UVoiceChatComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UVoiceChatComponent();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    //////////////////////////////////////////////////////////////////////////
    // Public API
    //////////////////////////////////////////////////////////////////////////

    /** Initialize voice chat system (EOS login + voice connect) */
    UFUNCTION(BlueprintCallable, Category = "Voice Chat")
    void Initialize();

    /** Join a voice channel using server-provided credentials */
    UFUNCTION(BlueprintCallable, Category = "Voice Chat")
    void JoinChannel(const FString& ChannelName, const FString& Token, bool bPositional = false);

    /** Leave current voice channel */
    UFUNCTION(BlueprintCallable, Category = "Voice Chat")
    void LeaveChannel();

    /** Get local user's Product User ID (send this to server to request credentials) */
    UFUNCTION(BlueprintPure, Category = "Voice Chat")
    FString GetProductUserId() const;

    /** Check if voice system is ready */
    UFUNCTION(BlueprintPure, Category = "Voice Chat")
    bool IsReady() const { return bIsReady; }

    /** Get current channel name */
    UFUNCTION(BlueprintPure, Category = "Voice Chat")
    FString GetCurrentChannel() const { return CurrentChannel; }

    //////////////////////////////////////////////////////////////////////////
    // Audio Control
    //////////////////////////////////////////////////////////////////////////

    UFUNCTION(BlueprintCallable, Category = "Voice Chat|Audio")
    void SetMuted(bool bMuted);

    UFUNCTION(BlueprintPure, Category = "Voice Chat|Audio")
    bool IsMuted() const;

    UFUNCTION(BlueprintCallable, Category = "Voice Chat|Audio")
    void SetInputVolume(float Volume);

    UFUNCTION(BlueprintCallable, Category = "Voice Chat|Audio")
    void SetOutputVolume(float Volume);

    //////////////////////////////////////////////////////////////////////////
    // Events
    //////////////////////////////////////////////////////////////////////////

    UPROPERTY(BlueprintAssignable, Category = "Voice Chat|Events")
    FOnVoiceChatReady OnReady;

    UPROPERTY(BlueprintAssignable, Category = "Voice Chat|Events")
    FOnVoiceChannelJoined OnChannelJoined;

    UPROPERTY(BlueprintAssignable, Category = "Voice Chat|Events")
    FOnVoiceChannelLeft OnChannelLeft;

protected:
    /** Auto-initialize on BeginPlay */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voice Chat")
    bool bAutoInitialize = true;

    /** Auto-join main channel when voice is ready */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voice Chat")
    bool bAutoJoinMainChannel = true;

    /** Main channel name to auto-join */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voice Chat")
    FString MainChannelName = TEXT("MainChannel");

    /** Auto-leave channel on destroy */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voice Chat")
    bool bAutoLeaveOnDestroy = true;

private:
    // EOS Authentication
    void AuthenticateEOS();
    void OnEOSAuthComplete(int32 LocalUserNum, bool bSuccess, const FUniqueNetId& UserId, const FString& Error);

    // Voice Chat Setup (following EOSIntegrationKit pattern)
    void InitializeVoiceChat();
    void ConnectVoiceChat();
    void OnVoiceConnectComplete(const FVoiceChatResult& Result);
    void LoginVoiceUser();
    void OnVoiceUserLoginComplete(const FString& UserName, const FVoiceChatResult& Result);

    // Channel Management
    void RequestVoiceCredentialsFromServer();
    void OnChannelJoinComplete(const FString& ChannelName, const FVoiceChatResult& Result);
    void OnChannelLeaveComplete(const FString& ChannelName, const FVoiceChatResult& Result);

    // Cleanup
    void Cleanup();

private:
    IVoiceChat* VoiceChat = nullptr;
    IVoiceChatUser* VoiceUser = nullptr;

    TSharedPtr<const FUniqueNetId> EOSUserId;
    FString ProductUserId;
    FString CurrentChannel;

    bool bIsReady = false;
    bool bIsInitializing = false;
    bool bIsMuted = false;
};
