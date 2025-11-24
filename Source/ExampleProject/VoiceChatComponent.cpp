// VoiceChatComponent.cpp
#include "VoiceChatComponent.h"

#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemTypes.h"

namespace VoiceChatComponentInternal
{
    static const FName VoiceLobbyFlag(TEXT("VOICE_LOBBY"));
}

UVoiceChatComponent::UVoiceChatComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UVoiceChatComponent::BeginPlay()
{
    Super::BeginPlay();

    if (bAutoStartOnBeginPlay)
    {
        //StartVoiceChat();
    }
}

void UVoiceChatComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (bTearDownSessionOnEndPlay)
    {
        LeaveVoiceLobby();
    }
    else if (bLeaveVoiceOnDestroy && !CurrentVoiceChannel.IsEmpty())
    {
        LeaveVoiceChannel(CurrentVoiceChannel);
    }

    CleanupDelegates();

    if (VoiceUser)
    {
        VoiceUser->Logout(FOnVoiceChatLogoutCompleteDelegate::CreateUObject(this, &UVoiceChatComponent::OnVoiceLogoutComplete));
        VoiceUser = nullptr;
    }

    if (Voice && bVoiceInitialized)
    {
        Voice->Uninitialize();
    }

    Voice = nullptr;
    bVoiceInitialized = false;
    bVoiceLoginSucceeded = false;
    bEOSLoginInProgress = false;

    SessionInterface.Reset();
    ResetSessionSearch();

    Super::EndPlay(EndPlayReason);
}

void UVoiceChatComponent::StartVoiceChat()
{
    if (bVoiceLoginSucceeded)
    {
        HandleAutoLobbyAction();
        return;
    }

    if (bEOSLoginInProgress)
    {
        return;
    }

    if (PlayerName.IsEmpty())
    {
        if (const APlayerController* PC = Cast<APlayerController>(GetOwner()))
        {
            if (const ULocalPlayer* LocalPlayer = PC->GetLocalPlayer())
            {
                PlayerName = LocalPlayer->GetNickname();
                if (PlayerName.IsEmpty())
                {
                    PlayerName = LocalPlayer->GetName();
                }
            }
        }
    }

    if (PlayerName.IsEmpty())
    {
        PlayerName = TEXT("Player");
    }

    UE_LOG(LogTemp, Log, TEXT("[VoiceChat] Starting initialization for %s"), *PlayerName);

    bEOSLoginInProgress = true;
    LoginToEOS();
}

void UVoiceChatComponent::LoginToEOS()
{
    IOnlineSubsystem* OSS = IOnlineSubsystem::Get();
    if (!OSS)
    {
        UE_LOG(LogTemp, Error, TEXT("[VoiceChat] OnlineSubsystem not found"));
        bEOSLoginInProgress = false;
        return;
    }

    IOnlineIdentityPtr Identity = OSS->GetIdentityInterface();
    if (!Identity.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("[VoiceChat] Identity interface invalid"));
        bEOSLoginInProgress = false;
        return;
    }

    if (Identity->GetLoginStatus(0) == ELoginStatus::LoggedIn)
    {
        UE_LOG(LogTemp, Log, TEXT("[VoiceChat] Already logged into EOS"));
        bEOSLoginInProgress = false;
        InitializeVoiceChat();
        return;
    }

    FOnlineAccountCredentials Credentials;
    Credentials.Type = TEXT("accountportal");
    Credentials.Id = FString();
    Credentials.Token = FString();

    Identity->OnLoginCompleteDelegates->AddUObject(this, &UVoiceChatComponent::OnEOSLoginComplete);

    UE_LOG(LogTemp, Log, TEXT("[VoiceChat] Attempting EOS login..."));
    if (!Identity->Login(0, Credentials))
    {
        UE_LOG(LogTemp, Error, TEXT("[VoiceChat] Identity->Login returned false immediately"));
        Identity->OnLoginCompleteDelegates->RemoveAll(this);
        bEOSLoginInProgress = false;
    }
}

void UVoiceChatComponent::OnEOSLoginComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error)
{
    bEOSLoginInProgress = false;

    if (IOnlineSubsystem* OSS = IOnlineSubsystem::Get())
    {
        if (IOnlineIdentityPtr Identity = OSS->GetIdentityInterface())
        {
            Identity->OnLoginCompleteDelegates->RemoveAll(this);
        }
    }

    if (bWasSuccessful)
    {
        UE_LOG(LogTemp, Log, TEXT("[VoiceChat] EOS Login Successful: %s"), *UserId.ToString());
        InitializeVoiceChat();
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[VoiceChat] EOS Login Failed: %s"), *Error);
    }
}

void UVoiceChatComponent::InitializeVoiceChat()
{
    if (!Voice)
    {
        Voice = IVoiceChat::Get();
    }

    if (!Voice)
    {
        UE_LOG(LogTemp, Error, TEXT("[VoiceChat] VoiceChat interface unavailable"));
        return;
    }

    if (!bVoiceInitialized)
    {
        bVoiceInitialized = Voice->Initialize();
        if (!bVoiceInitialized)
        {
            UE_LOG(LogTemp, Error, TEXT("[VoiceChat] VoiceChat Initialize failed"));
            return;
        }
    }

    LoginToVoice();
}

void UVoiceChatComponent::LoginToVoice()
{
    if (!Voice)
    {
        UE_LOG(LogTemp, Error, TEXT("[VoiceChat] Voice interface missing"));
        return;
    }

    if (!VoiceUser)
    {
        VoiceUser = Voice->CreateUser();
    }

    IOnlineSubsystem* OSS = IOnlineSubsystem::Get();
    if (!OSS)
    {
        UE_LOG(LogTemp, Error, TEXT("[VoiceChat] OnlineSubsystem not found"));
        return;
    }

    IOnlineIdentityPtr Identity = OSS->GetIdentityInterface();
    if (!Identity.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("[VoiceChat] Identity interface invalid"));
        return;
    }

    LocalUserId = Identity->GetUniquePlayerId(0);
    if (!LocalUserId.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("[VoiceChat] Invalid LocalUserId"));
        return;
    }

    const FPlatformUserId PlatformUserId = Identity->GetPlatformUserIdFromUniqueNetId(*LocalUserId);

    UE_LOG(LogTemp, Log, TEXT("[VoiceChat] Logging into Voice as %s"), *LocalUserId->ToString());
    HandleAutoLobbyAction();

}

void UVoiceChatComponent::OnVoiceLoginComplete(const FString& UserName, const FVoiceChatResult& Result)
{
    if (Result.IsSuccess())
    {
        bVoiceLoginSucceeded = true;
        UE_LOG(LogTemp, Log, TEXT("[VoiceChat] Voice login successful for %s"), *UserName);
        HandleAutoLobbyAction();
    }
    else
    {
        bVoiceLoginSucceeded = false;
        UE_LOG(LogTemp, Error, TEXT("[VoiceChat] Voice login failed for %s: %s"), *UserName, *Result.ErrorCode);
    }
}

void UVoiceChatComponent::OnVoiceLogoutComplete(const FString& UserName, const FVoiceChatResult& Result)
{
    UE_LOG(LogTemp, Log, TEXT("[VoiceChat] Voice logout complete for %s (Success: %s)"),
        *UserName,
        Result.IsSuccess() ? TEXT("true") : *Result.ErrorCode);
}

void UVoiceChatComponent::HostVoiceLobby()
{
    if (!bVoiceLoginSucceeded)
    {
        bPendingHostRequest = true;
        StartVoiceChat();
        return;
    }

    bPendingHostRequest = false;
    HostLobbyInternal();
}

void UVoiceChatComponent::FindAndJoinVoiceLobby()
{
    if (!bVoiceLoginSucceeded)
    {
        bPendingJoinRequest = true;
        StartVoiceChat();
        return;
    }

    bPendingJoinRequest = false;
    FindLobbyInternal();
}

void UVoiceChatComponent::LeaveVoiceLobby()
{
    if (!CurrentVoiceChannel.IsEmpty())
    {
        LeaveVoiceChannel(CurrentVoiceChannel);
    }

    EnsureSessionInterface();
    if (SessionInterface.IsValid() && SessionInterface->GetNamedSession(LobbySessionName))
    {
        DestroySessionCompleteHandle = SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(
            FOnDestroySessionCompleteDelegate::CreateUObject(this, &UVoiceChatComponent::OnDestroySessionComplete));

        if (!SessionInterface->DestroySession(LobbySessionName))
        {
            SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteHandle);
            UE_LOG(LogTemp, Warning, TEXT("[VoiceChat] DestroySession request rejected"));
        }
    }
}

void UVoiceChatComponent::JoinVoiceChannel(const FString& ChannelName)
{
    if (!VoiceUser || ChannelName.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("[VoiceChat] Unable to join voice channel (Voice=%p, Name=%s)"),
            VoiceUser,
            *ChannelName);
        return;
    }

    CurrentVoiceChannel = ChannelName;

    UE_LOG(LogTemp, Log, TEXT("[VoiceChat] Joining voice channel: %s"), *ChannelName);
    VoiceUser->JoinChannel(
        ChannelName,
        TEXT(""),
        EVoiceChatChannelType::NonPositional,
        FOnVoiceChatChannelJoinCompleteDelegate::CreateUObject(this, &UVoiceChatComponent::OnChannelJoinComplete));
}

void UVoiceChatComponent::JoinVoiceChannelForSession(FName SessionName)
{
    FString ChannelName = ManualChannelName;

    EnsureSessionInterface();
    if (ChannelName.IsEmpty() && SessionInterface.IsValid())
    {
        if (FNamedOnlineSession* NamedSession = SessionInterface->GetNamedSession(SessionName))
        {
            ChannelName = NamedSession->GetSessionIdStr();
        }
    }

    if (ChannelName.IsEmpty())
    {
        ChannelName = SessionName.ToString();
    }

    JoinVoiceChannel(ChannelName);
}

void UVoiceChatComponent::OnChannelJoinComplete(const FString& ChannelName, const FVoiceChatResult& Result)
{
    if (Result.IsSuccess())
    {
        UE_LOG(LogTemp, Log, TEXT("[VoiceChat] Joined voice channel %s"), *ChannelName);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[VoiceChat] Failed to join channel %s: %s"), *ChannelName, *Result.ErrorCode);
    }
}

void UVoiceChatComponent::LeaveVoiceChannel(const FString& ChannelName)
{
    if (!VoiceUser || ChannelName.IsEmpty())
    {
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("[VoiceChat] Leaving voice channel %s"), *ChannelName);
    VoiceUser->LeaveChannel(
        ChannelName,
        FOnVoiceChatChannelLeaveCompleteDelegate::CreateUObject(this, &UVoiceChatComponent::OnChannelLeaveComplete));
}

void UVoiceChatComponent::OnChannelLeaveComplete(const FString& ChannelName, const FVoiceChatResult& Result)
{
    if (Result.IsSuccess())
    {
        UE_LOG(LogTemp, Log, TEXT("[VoiceChat] Left voice channel %s"), *ChannelName);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[VoiceChat] Failed to leave channel %s: %s"), *ChannelName, *Result.ErrorCode);
    }

    if (ChannelName == CurrentVoiceChannel)
    {
        CurrentVoiceChannel.Empty();
    }
}

void UVoiceChatComponent::HandleAutoLobbyAction()
{
    if (bPendingHostRequest)
    {
        bPendingHostRequest = false;
        HostLobbyInternal();
        return;
    }

    if (bPendingJoinRequest)
    {
        bPendingJoinRequest = false;
        FindLobbyInternal();
        return;
    }

    switch (AutoLobbyStrategy)
    {
    case EVoiceLobbyJoinStrategy::Host:
        HostLobbyInternal();
        break;
    case EVoiceLobbyJoinStrategy::FindExisting:
        FindLobbyInternal();
        break;
    default:
        break;
    }
}

bool UVoiceChatComponent::EnsureVoiceReady(const FString& ContextMessage)
{

    EnsureSessionInterface();
    if (!SessionInterface.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("[VoiceChat] %s: Session interface unavailable"), *ContextMessage);
        return false;
    }

    return true;
}

void UVoiceChatComponent::EnsureSessionInterface()
{
    if (SessionInterface.IsValid())
    {
        return;
    }

    if (IOnlineSubsystem* OSS = IOnlineSubsystem::Get())
    {
        SessionInterface = OSS->GetSessionInterface();
    }
}

void UVoiceChatComponent::HostLobbyInternal()
{
    if (!EnsureVoiceReady(TEXT("HostLobby")))
    {
        UE_LOG(LogTemp, Log, TEXT("[VoiceChat] Could start session"));
    }

    if (SessionInterface->GetNamedSession(LobbySessionName))
    {
        UE_LOG(LogTemp, Log, TEXT("[VoiceChat] Session already exists, joining voice directly"));
        JoinVoiceChannelForSession(LobbySessionName);
        return;
    }

    FOnlineSessionSettings SessionSettings;
    SessionSettings.bIsLANMatch = false;
    SessionSettings.bShouldAdvertise = true;
    SessionSettings.bAllowJoinInProgress = true;
    SessionSettings.bAllowJoinViaPresence = true;
    SessionSettings.bAllowInvites = true;
    SessionSettings.bUsesPresence = true;
    SessionSettings.bUseLobbiesIfAvailable = true;
    SessionSettings.bUseLobbiesVoiceChatIfAvailable = true;
    SessionSettings.NumPublicConnections = MaxLobbySize;
    SessionSettings.Set(VoiceChatComponentInternal::VoiceLobbyFlag, FString(TEXT("1")), EOnlineDataAdvertisementType::ViaOnlineService);


    CreateSessionCompleteHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(
        FOnCreateSessionCompleteDelegate::CreateUObject(this, &UVoiceChatComponent::OnCreateSessionComplete));

    if (!SessionInterface->CreateSession(0, LobbySessionName, SessionSettings))
    {
        SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteHandle);
        UE_LOG(LogTemp, Error, TEXT("[VoiceChat] Failed to start CreateSession"));
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("[VoiceChat] CreateSession completed for %s"), *LobbySessionName.ToString());
    }
}

void UVoiceChatComponent::FindLobbyInternal()
{
    if (!EnsureVoiceReady(TEXT("FindLobby")))
    {
        UE_LOG(LogTemp, Log, TEXT("[VoiceChat] Could start session"));
    }

    if (SessionSearch.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[VoiceChat] Lobby search already running"));
        return;
    }

    SessionSearch = MakeShared<FOnlineSessionSearch>();
    SessionSearch->MaxSearchResults = 50;
    SessionSearch->bIsLanQuery = false;
    SessionSearch->QuerySettings.Set(VoiceChatComponentInternal::VoiceLobbyFlag, FString(TEXT("1")), EOnlineComparisonOp::Equals);


    FindSessionsCompleteHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(
        FOnFindSessionsCompleteDelegate::CreateUObject(this, &UVoiceChatComponent::OnFindSessionsComplete));

    if (!SessionInterface->FindSessions(0, SessionSearch.ToSharedRef()))
    {
        SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteHandle);
        UE_LOG(LogTemp, Error, TEXT("[VoiceChat] Failed to trigger FindSessions"));
        ResetSessionSearch();
    }
}

void UVoiceChatComponent::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
    if (!SessionInterface.IsValid())
    {
        return;
    }

    SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteHandle);

    if (bWasSuccessful)
    {
        UE_LOG(LogTemp, Log, TEXT("[VoiceChat] CreateSession completed for %s"), *SessionName.ToString());

        StartSessionCompleteHandle = SessionInterface->AddOnStartSessionCompleteDelegate_Handle(
            FOnStartSessionCompleteDelegate::CreateUObject(this, &UVoiceChatComponent::OnStartSessionComplete));

        if (!SessionInterface->StartSession(SessionName))
        {
            SessionInterface->ClearOnStartSessionCompleteDelegate_Handle(StartSessionCompleteHandle);
            UE_LOG(LogTemp, Error, TEXT("[VoiceChat] Failed to start session %s"), *SessionName.ToString());
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[VoiceChat] CreateSession failed for %s"), *SessionName.ToString());
    }
}

void UVoiceChatComponent::OnStartSessionComplete(FName SessionName, bool bWasSuccessful)
{
    if (!SessionInterface.IsValid())
    {
        return;
    }

    SessionInterface->ClearOnStartSessionCompleteDelegate_Handle(StartSessionCompleteHandle);

    if (bWasSuccessful)
    {
        UE_LOG(LogTemp, Log, TEXT("[VoiceChat] Session %s started, joining voice channel"), *SessionName.ToString());
        JoinVoiceChannelForSession(SessionName);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[VoiceChat] Failed to start session %s"), *SessionName.ToString());
    }
}

void UVoiceChatComponent::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
    if (!SessionInterface.IsValid())
    {
        return;
    }

    SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteHandle);

    UE_LOG(LogTemp, Log, TEXT("[VoiceChat] DestroySession %s result: %s"),
        *SessionName.ToString(),
        bWasSuccessful ? TEXT("Success") : TEXT("Failure"));
}

void UVoiceChatComponent::OnFindSessionsComplete(bool bWasSuccessful)
{
    if (!SessionInterface.IsValid())
    {
        return;
    }

    SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteHandle);

    if (!bWasSuccessful || !SessionSearch.IsValid() || SessionSearch->SearchResults.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[VoiceChat] Lobby search finished without results"));
        ResetSessionSearch();
        return;
    }

    const FOnlineSessionSearchResult& Result = SessionSearch->SearchResults[0];

    JoinSessionCompleteHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(
        FOnJoinSessionCompleteDelegate::CreateUObject(this, &UVoiceChatComponent::OnJoinSessionComplete));

    if (!SessionInterface->JoinSession(0, LobbySessionName, Result))
    {
        SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteHandle);
        UE_LOG(LogTemp, Error, TEXT("[VoiceChat] JoinSession request rejected"));
        ResetSessionSearch();
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("[VoiceChat] JoinSession completed for %s"), *LobbySessionName.ToString());
    }
}

void UVoiceChatComponent::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
    if (!SessionInterface.IsValid())
    {
        return;
    }

    SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteHandle);

    if (Result == EOnJoinSessionCompleteResult::Success)
    {
        UE_LOG(LogTemp, Log, TEXT("[VoiceChat] Joined session %s, joining voice channel"), *SessionName.ToString());
        JoinVoiceChannelForSession(SessionName);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[VoiceChat] JoinSession failed for %s (code %d)"), *SessionName.ToString(), static_cast<int32>(Result));
    }

    ResetSessionSearch();
}

void UVoiceChatComponent::ResetSessionSearch()
{
    if (SessionSearch.IsValid())
    {
        SessionSearch.Reset();
    }
}

void UVoiceChatComponent::CleanupDelegates()
{
    if (!SessionInterface.IsValid())
    {
        return;
    }

    if (CreateSessionCompleteHandle.IsValid())
    {
        SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteHandle);
    }
    if (StartSessionCompleteHandle.IsValid())
    {
        SessionInterface->ClearOnStartSessionCompleteDelegate_Handle(StartSessionCompleteHandle);
    }
    if (DestroySessionCompleteHandle.IsValid())
    {
        SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteHandle);
    }
    if (FindSessionsCompleteHandle.IsValid())
    {
        SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteHandle);
    }
    if (JoinSessionCompleteHandle.IsValid())
    {
        SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteHandle);
    }

    ResetSessionSearch();
}