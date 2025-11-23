// Fill out your copyright notice in the Description page of Project Settings.
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "OnlineSubsystem.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "VoiceChat.h"
#include "VoiceChatComponent.generated.h"

UENUM(BlueprintType)
enum class EVoiceLobbyJoinStrategy : uint8
{
    None UMETA(DisplayName = "Do Nothing"),
    Host UMETA(DisplayName = "Create Lobby"),
    FindExisting UMETA(DisplayName = "Find & Join Lobby")
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class EXAMPLEPROJECT_API UVoiceChatComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UVoiceChatComponent();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    /** Starts the full EOS voice chat pipeline (login + voice init). */
    UFUNCTION(BlueprintCallable, Category = "Voice Chat|EOS")
    void StartVoiceChat();

    /** Hosts a voice-enabled EOS lobby (uses EOS lobbies under the hood). */
    UFUNCTION(BlueprintCallable, Category = "Voice Chat|EOS")
    void HostVoiceLobby();

    /** Finds the best matching lobby and joins it, then hooks up voice chat. */
    UFUNCTION(BlueprintCallable, Category = "Voice Chat|EOS")
    void FindAndJoinVoiceLobby();

    /** Leaves the active voice channel and destroys/cleans up the lobby session. */
    UFUNCTION(BlueprintCallable, Category = "Voice Chat|EOS")
    void LeaveVoiceLobby();

    /** Returns true when the local user finished logging in to EOS Voice services. */
    UFUNCTION(BlueprintPure, Category = "Voice Chat|EOS")
    bool IsVoiceChatReady() const { return bVoiceLoginSucceeded; }

protected:
    /** Automatically bootstrap voice on BeginPlay. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voice Chat|EOS")
    bool bAutoStartOnBeginPlay = true;

    /** Optional automatic lobby workflow once voice login completes. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voice Chat|EOS")
    EVoiceLobbyJoinStrategy AutoLobbyStrategy = EVoiceLobbyJoinStrategy::Host;

    /** Name used for CreateSession/JoinSession. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voice Chat|EOS")
    FName LobbySessionName = TEXT("VoiceLobby");

    /** Keyword/Bucket used to match lobbies cross-platform. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voice Chat|EOS")
    FString LobbyKeyword = TEXT("ExampleProjectLobby");

    /** Maximum number of slots available in the generated lobby. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voice Chat|EOS", meta = (ClampMin = "2", ClampMax = "100"))
    int32 MaxLobbySize = 16;

    /** Optional manual channel name override (otherwise we use the EOS Lobby/Session id). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voice Chat|EOS")
    FString ManualChannelName;

    /** Should the component automatically destroy the hosted session/lobby on EndPlay. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voice Chat|EOS")
    bool bTearDownSessionOnEndPlay = true;

    /** Whether to auto-leave the voice channel when the component shuts down. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Voice Chat|EOS")
    bool bLeaveVoiceOnDestroy = true;

private:
    // EOS Identity Login
    void LoginToEOS();
    void OnEOSLoginComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error);

    // Voice Chat Functions
    void InitializeVoiceChat();
    void LoginToVoice();
    void OnVoiceLoginComplete(const FString& UserName, const FVoiceChatResult& Result);
    void OnVoiceLogoutComplete(const FString& UserName, const FVoiceChatResult& Result);

    // Channel Functions
    void JoinVoiceChannel(const FString& ChannelName);
    void JoinVoiceChannelForSession(FName SessionName);
    void OnChannelJoinComplete(const FString& ChannelName, const FVoiceChatResult& Result);
    void LeaveVoiceChannel(const FString& ChannelName);
    void OnChannelLeaveComplete(const FString& ChannelName, const FVoiceChatResult& Result);

    // Lobby/Session helpers
    void HandleAutoLobbyAction();
    bool EnsureVoiceReady(const FString& ContextMessage);
    void EnsureSessionInterface();
    void HostLobbyInternal();
    void FindLobbyInternal();
    void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
    void OnStartSessionComplete(FName SessionName, bool bWasSuccessful);
    void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);
    void OnFindSessionsComplete(bool bWasSuccessful);
    void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
    void ResetSessionSearch();
    void CleanupDelegates();

private:
    IVoiceChat* Voice = nullptr;
    IVoiceChatUser* VoiceUser = nullptr;
    IOnlineSessionPtr SessionInterface;
    TSharedPtr<FOnlineSessionSearch> SessionSearch;

    TSharedPtr<const FUniqueNetId> LocalUserId;
    FString PlayerName;
    FString CurrentVoiceChannel;

    FDelegateHandle CreateSessionCompleteHandle;
    FDelegateHandle StartSessionCompleteHandle;
    FDelegateHandle DestroySessionCompleteHandle;
    FDelegateHandle FindSessionsCompleteHandle;
    FDelegateHandle JoinSessionCompleteHandle;

    bool bVoiceInitialized = false;
    bool bVoiceLoginSucceeded = false;
    bool bEOSLoginInProgress = false;
    bool bPendingHostRequest = false;
    bool bPendingJoinRequest = false;
};