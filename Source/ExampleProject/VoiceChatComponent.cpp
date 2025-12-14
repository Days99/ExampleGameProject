// VoiceChatComponent.cpp
// Simplified EOS Voice Chat for Trusted Server Method
// Based on EOSIntegrationKit pattern: https://github.com/betidestudio/EOSIntegrationKit

#include "VoiceChatComponent.h"
#include "Engine/World.h"
#include "OnlineSubsystem.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Core/ExampleProjectPlayerController.h"

UVoiceChatComponent::UVoiceChatComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UVoiceChatComponent::BeginPlay()
{
    Super::BeginPlay();

    if (bAutoInitialize)
    {
        Initialize();
    }
}

void UVoiceChatComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (bAutoLeaveOnDestroy && !CurrentChannel.IsEmpty())
    {
        LeaveChannel();
    }

    Cleanup();
    Super::EndPlay(EndPlayReason);
}

//////////////////////////////////////////////////////////////////////////
// Public API
//////////////////////////////////////////////////////////////////////////

void UVoiceChatComponent::Initialize()
{
    if (bIsReady)
    {
        UE_LOG(LogTemp, Log, TEXT("[VoiceChat] Already initialized"));
        OnReady.Broadcast();
        return;
    }

    if (bIsInitializing)
    {
        UE_LOG(LogTemp, Warning, TEXT("[VoiceChat] Already initializing"));
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("[VoiceChat] ======================================"));
    UE_LOG(LogTemp, Log, TEXT("[VoiceChat] Initializing Voice Chat System"));
    UE_LOG(LogTemp, Log, TEXT("[VoiceChat] ======================================"));

    bIsInitializing = true;
    AuthenticateEOS();
}

void UVoiceChatComponent::JoinChannel(const FString& ChannelName, const FString& Token, bool bPositional)
{
    if (!bIsReady || !VoiceUser)
    {
        UE_LOG(LogTemp, Error, TEXT("[VoiceChat] Cannot join channel - voice not ready"));
        OnChannelJoined.Broadcast(ChannelName, false);
        return;
    }

    if (ChannelName.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("[VoiceChat] Channel name cannot be empty"));
        OnChannelJoined.Broadcast(ChannelName, false);
        return;
    }

    // Leave current channel if different
    if (!CurrentChannel.IsEmpty() && CurrentChannel != ChannelName)
    {
        LeaveChannel();
    }

    CurrentChannel = ChannelName;

    EVoiceChatChannelType ChannelType = bPositional ? EVoiceChatChannelType::Positional : EVoiceChatChannelType::NonPositional;

    UE_LOG(LogTemp, Log, TEXT("[VoiceChat] Joining channel: %s (Token: %s)"), 
        *ChannelName, 
        Token.IsEmpty() ? TEXT("None") : TEXT("Provided"));

    VoiceUser->JoinChannel(
        ChannelName,
        Token,
        ChannelType,
        FOnVoiceChatChannelJoinCompleteDelegate::CreateUObject(this, &UVoiceChatComponent::OnChannelJoinComplete)
    );
}

void UVoiceChatComponent::LeaveChannel()
{
    if (CurrentChannel.IsEmpty() || !VoiceUser)
    {
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("[VoiceChat] Leaving channel: %s"), *CurrentChannel);

    VoiceUser->LeaveChannel(
        CurrentChannel,
        FOnVoiceChatChannelLeaveCompleteDelegate::CreateUObject(this, &UVoiceChatComponent::OnChannelLeaveComplete)
    );
}

void UVoiceChatComponent::SetMuted(bool bMuted)
{
    bIsMuted = bMuted;
    if (VoiceUser)
    {
        VoiceUser->SetAudioInputDeviceMuted(bMuted);
        UE_LOG(LogTemp, Log, TEXT("[VoiceChat] Mute: %s"), bMuted ? TEXT("ON") : TEXT("OFF"));
    }
}

bool UVoiceChatComponent::IsMuted() const
{
    return VoiceUser ? VoiceUser->GetAudioInputDeviceMuted() : bIsMuted;
}

void UVoiceChatComponent::SetInputVolume(float Volume)
{
    if (VoiceUser)
    {
        VoiceUser->SetAudioInputVolume(FMath::Clamp(Volume, 0.0f, 1.0f));
    }
}

void UVoiceChatComponent::SetOutputVolume(float Volume)
{
    if (VoiceUser)
    {
        VoiceUser->SetAudioOutputVolume(FMath::Clamp(Volume, 0.0f, 1.0f));
    }
}

FString UVoiceChatComponent::GetProductUserId() const
{
    // Extract just the ProductUserId part (after the pipe if present)
    // EOS format can be: "AccountId|ProductUserId" or just "ProductUserId"
    // Server needs just the ProductUserId part for EOS_RTCAdmin_QueryJoinRoomToken
    int32 PipeIndex;
    if (ProductUserId.FindChar(TEXT('|'), PipeIndex))
    {
        return ProductUserId.Mid(PipeIndex + 1);
    }
    return ProductUserId;
}

//////////////////////////////////////////////////////////////////////////
// EOS Authentication
//////////////////////////////////////////////////////////////////////////

void UVoiceChatComponent::AuthenticateEOS()
{
    IOnlineSubsystem* OSS = IOnlineSubsystem::Get();
    if (!OSS)
    {
        UE_LOG(LogTemp, Error, TEXT("[VoiceChat] OnlineSubsystem not available"));
        bIsInitializing = false;
        return;
    }

    IOnlineIdentityPtr Identity = OSS->GetIdentityInterface();
    if (!Identity.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("[VoiceChat] Identity interface not available"));
        bIsInitializing = false;
        return;
    }

    // Check if already authenticated
    if (Identity->GetLoginStatus(0) == ELoginStatus::LoggedIn)
    {
        UE_LOG(LogTemp, Log, TEXT("[VoiceChat] Already authenticated with EOS"));
        EOSUserId = Identity->GetUniquePlayerId(0);
        if (EOSUserId.IsValid())
        {
            ProductUserId = EOSUserId->ToString();
        }
        InitializeVoiceChat();
        return;
    }

    // Authenticate with Account Portal
    FOnlineAccountCredentials Credentials;
    Credentials.Type = TEXT("accountportal");
    Credentials.Id = FString();
    Credentials.Token = FString();

    Identity->OnLoginCompleteDelegates->AddUObject(this, &UVoiceChatComponent::OnEOSAuthComplete);

    UE_LOG(LogTemp, Log, TEXT("[VoiceChat] Authenticating with EOS (Account Portal)..."));

    if (!Identity->Login(0, Credentials))
    {
        UE_LOG(LogTemp, Error, TEXT("[VoiceChat] Login request failed"));
        Identity->OnLoginCompleteDelegates->RemoveAll(this);
        bIsInitializing = false;
    }
}

void UVoiceChatComponent::OnEOSAuthComplete(int32 LocalUserNum, bool bSuccess, const FUniqueNetId& UserId, const FString& Error)
{
    // Cleanup delegate
    if (IOnlineSubsystem* OSS = IOnlineSubsystem::Get())
    {
        if (IOnlineIdentityPtr Identity = OSS->GetIdentityInterface())
        {
            Identity->OnLoginCompleteDelegates->RemoveAll(this);
        }
    }

    if (bSuccess)
    {
        // Get the shared pointer from Identity interface (can't copy FUniqueNetId - it's abstract)
        if (IOnlineSubsystem* OSS = IOnlineSubsystem::Get())
        {
            if (IOnlineIdentityPtr Identity = OSS->GetIdentityInterface())
            {
                EOSUserId = Identity->GetUniquePlayerId(LocalUserNum);
                if (EOSUserId.IsValid())
                {
                    ProductUserId = EOSUserId->ToString();
                }
                else
                {
                    // Fallback to string representation from callback
                    ProductUserId = UserId.ToString();
                }
            }
        }
        else
        {
            // Fallback to string representation from callback
            ProductUserId = UserId.ToString();
        }

        UE_LOG(LogTemp, Log, TEXT("[VoiceChat] EOS Authentication SUCCESS"));
        UE_LOG(LogTemp, Log, TEXT("[VoiceChat] ProductUserId: %s"), *ProductUserId);
        InitializeVoiceChat();
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[VoiceChat] EOS Authentication FAILED: %s"), *Error);
        bIsInitializing = false;
    }
}

//////////////////////////////////////////////////////////////////////////
// Voice Chat Setup (Following EOSIntegrationKit Pattern)
//////////////////////////////////////////////////////////////////////////

void UVoiceChatComponent::InitializeVoiceChat()
{
    // Get voice chat interface (following EOSIntegrationKit pattern)
    VoiceChat = IVoiceChat::Get();
    if (!VoiceChat)
    {
        UE_LOG(LogTemp, Error, TEXT("[VoiceChat] IVoiceChat::Get() returned null - is EOSVoiceChat plugin enabled?"));
        bIsInitializing = false;
        return;
    }

    // Initialize voice chat
    if (!VoiceChat->Initialize())
    {
        UE_LOG(LogTemp, Error, TEXT("[VoiceChat] VoiceChat->Initialize() failed"));
        bIsInitializing = false;
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("[VoiceChat] Voice chat initialized"));

    // Connect to voice service (following EOSIntegrationKit pattern)
    ConnectVoiceChat();
}

void UVoiceChatComponent::ConnectVoiceChat()
{
    if (!VoiceChat)
    {
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("[VoiceChat] Connecting to voice service..."));

    // Connect (following EOSIntegrationKit: EVIK_Local_Connect pattern)
    VoiceChat->Connect(FOnVoiceChatConnectCompleteDelegate::CreateUObject(this, &UVoiceChatComponent::OnVoiceConnectComplete));
}

void UVoiceChatComponent::OnVoiceConnectComplete(const FVoiceChatResult& Result)
{
    if (Result.IsSuccess())
    {
        UE_LOG(LogTemp, Log, TEXT("[VoiceChat] Voice service connected"));
        
        // Create user and login (following EOSIntegrationKit pattern)
        LoginVoiceUser();
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[VoiceChat] Voice connect FAILED: %s"), *Result.ErrorCode);
        bIsInitializing = false;
    }
}

void UVoiceChatComponent::LoginVoiceUser()
{
    if (!VoiceChat)
    {
        return;
    }

    // Create voice user (following EOSIntegrationKit pattern)
    if (!VoiceUser)
    {
        VoiceUser = VoiceChat->CreateUser();
        if (!VoiceUser)
        {
            UE_LOG(LogTemp, Error, TEXT("[VoiceChat] Failed to create VoiceUser"));
            bIsInitializing = false;
            return;
        }
    }

    // Get platform user ID for login
    IOnlineSubsystem* OSS = IOnlineSubsystem::Get();
    if (!OSS || !EOSUserId.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("[VoiceChat] Cannot login - invalid state"));
        bIsInitializing = false;
        return;
    }

    IOnlineIdentityPtr Identity = OSS->GetIdentityInterface();
    if (!Identity.IsValid())
    {
        bIsInitializing = false;
        return;
    }

    FPlatformUserId PlatformUserId = Identity->GetPlatformUserIdFromUniqueNetId(*EOSUserId);

    // Extract just the ProductUserId part (after the pipe if present)
    // EOS ProductUserId format can be: "AccountId|ProductUserId" or just "ProductUserId"
    FString VoiceLoginUserId = ProductUserId;
    int32 PipeIndex;
    if (ProductUserId.FindChar(TEXT('|'), PipeIndex))
    {
        // Extract the part after the pipe (the actual ProductUserId)
        VoiceLoginUserId = ProductUserId.Mid(PipeIndex + 1);
        UE_LOG(LogTemp, Log, TEXT("[VoiceChat] Extracted ProductUserId from full ID: %s -> %s"), *ProductUserId, *VoiceLoginUserId);
    }

    UE_LOG(LogTemp, Log, TEXT("[VoiceChat] Logging in voice user: %s"), *VoiceLoginUserId);

    // Login user with Product User ID (must not contain pipe character)
    VoiceUser->Login(
        PlatformUserId,
        VoiceLoginUserId,
        TEXT(""),
        FOnVoiceChatLoginCompleteDelegate::CreateUObject(this, &UVoiceChatComponent::OnVoiceUserLoginComplete)
    );
}

void UVoiceChatComponent::OnVoiceUserLoginComplete(const FString& UserName, const FVoiceChatResult& Result)
{
    if (Result.IsSuccess())
    {
        bIsReady = true;
        bIsInitializing = false;

        UE_LOG(LogTemp, Log, TEXT("[VoiceChat] ======================================"));
        UE_LOG(LogTemp, Log, TEXT("[VoiceChat] VOICE SYSTEM READY!"));
        UE_LOG(LogTemp, Log, TEXT("[VoiceChat] ProductUserId: %s"), *ProductUserId);
        UE_LOG(LogTemp, Log, TEXT("[VoiceChat] ======================================"));

        OnReady.Broadcast();

        // Check if we have pending credentials from server (received before voice was ready)
        if (APlayerController* PC = Cast<APlayerController>(GetOwner()))
        {
            if (AExampleProjectPlayerController* ExamplePC = Cast<AExampleProjectPlayerController>(PC))
            {
                if (!ExamplePC->PendingChannelName.IsEmpty() && !ExamplePC->PendingToken.IsEmpty())
                {
                    UE_LOG(LogTemp, Log, TEXT("[VoiceChat] Found pending credentials, joining channel: %s"), *ExamplePC->PendingChannelName);
                    JoinChannel(ExamplePC->PendingChannelName, ExamplePC->PendingToken, false);
                    // Clear pending credentials
                    ExamplePC->PendingChannelName.Empty();
                    ExamplePC->PendingToken.Empty();
                    return;
                }
            }
        }

        // Auto-join main channel if enabled - request credentials from server
        if (bAutoJoinMainChannel && !MainChannelName.IsEmpty())
        {
            RequestVoiceCredentialsFromServer();
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[VoiceChat] Voice user login FAILED: %s"), *Result.ErrorCode);
        bIsInitializing = false;
    }
}

//////////////////////////////////////////////////////////////////////////
// Channel Management
//////////////////////////////////////////////////////////////////////////

void UVoiceChatComponent::RequestVoiceCredentialsFromServer()
{
    UE_LOG(LogTemp, Log, TEXT("[VoiceChat] Requesting voice credentials from server for channel: %s"), *MainChannelName);
    
    // Get PlayerController to call server RPC
    if (APlayerController* PC = Cast<APlayerController>(GetOwner()))
    {
        if (AExampleProjectPlayerController* ExamplePC = Cast<AExampleProjectPlayerController>(PC))
        {
            FString ProductUserIdToSend = GetProductUserId();
            ExamplePC->Server_RequestVoiceCredentials(ProductUserIdToSend);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("[VoiceChat] PlayerController is not AExampleProjectPlayerController"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[VoiceChat] Owner is not a PlayerController"));
    }
}

//////////////////////////////////////////////////////////////////////////
// Channel Callbacks
//////////////////////////////////////////////////////////////////////////

void UVoiceChatComponent::OnChannelJoinComplete(const FString& ChannelName, const FVoiceChatResult& Result)
{
    if (Result.IsSuccess())
    {
        UE_LOG(LogTemp, Log, TEXT("[VoiceChat] JOINED channel: %s"), *ChannelName);
        OnChannelJoined.Broadcast(ChannelName, true);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[VoiceChat] FAILED to join channel %s: %s"), *ChannelName, *Result.ErrorCode);
        CurrentChannel.Empty();
        OnChannelJoined.Broadcast(ChannelName, false);
    }
}

void UVoiceChatComponent::OnChannelLeaveComplete(const FString& ChannelName, const FVoiceChatResult& Result)
{
    if (Result.IsSuccess())
    {
        UE_LOG(LogTemp, Log, TEXT("[VoiceChat] LEFT channel: %s"), *ChannelName);
    }

    if (ChannelName == CurrentChannel)
    {
        CurrentChannel.Empty();
    }

    OnChannelLeft.Broadcast(ChannelName, Result.IsSuccess());
}

//////////////////////////////////////////////////////////////////////////
// Cleanup
//////////////////////////////////////////////////////////////////////////

void UVoiceChatComponent::Cleanup()
{
    if (VoiceUser)
    {
        VoiceUser->Logout(FOnVoiceChatLogoutCompleteDelegate::CreateLambda([](const FString&, const FVoiceChatResult&) {}));
        VoiceUser = nullptr;
    }

    if (VoiceChat)
    {
        VoiceChat->Disconnect(FOnVoiceChatDisconnectCompleteDelegate::CreateLambda([](const FVoiceChatResult&) {}));
        VoiceChat->Uninitialize();
        VoiceChat = nullptr;
    }

    bIsReady = false;
    bIsInitializing = false;
    CurrentChannel.Empty();
}
