#include "MatchmakingLevelScript.h"

#include "Blueprint/UserWidget.h"
#include "MatchmakingSubsystem.h"
#include "SteamSessionManager.h"
#include "BackendSettings.h"
#include "Core/ServerButton.h"
#include "Components/Button.h"
#include "Components/ScrollBox.h"
#include "Components/VerticalBox.h"
#include "Components/TextBlock.h"
#include "Components/EditableTextBox.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

void AMatchmakingLevelScript::BeginPlay() {
    Super::BeginPlay();

    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;

    if (UGameInstance* GI = GetGameInstance()) {
        MatchSubsystem = GI->GetSubsystem<UMatchmakingSubsystem>();
        SteamSessionMgr = GI->GetSubsystem<USteamSessionManager>();
        BackendSettingsSubsystem = GI->GetSubsystem<UBackendSettings>();

        if (MatchSubsystem) {
            MatchSubsystem->OnSessionsUpdated.AddDynamic(this, &AMatchmakingLevelScript::OnSessionsUpdated);
            MatchSubsystem->OnHostRequested.AddDynamic(this, &AMatchmakingLevelScript::OnHostRequested);
            MatchSubsystem->OnConnectionStatusChanged.AddDynamic(this, &AMatchmakingLevelScript::OnConnectionStatusChanged);
            MatchSubsystem->OnJoinSuccess.AddDynamic(this, &AMatchmakingLevelScript::OnJoinSuccess);
            MatchSubsystem->OnServerError.AddDynamic(this, &AMatchmakingLevelScript::OnServerError);
        }
    }

    if (MatchmakingWidgetClass) {
        MatchmakingWidget = CreateWidget<UUserWidget>(GetWorld(), MatchmakingWidgetClass);
        MatchmakingWidget->AddToViewport();

        if (UButton* ConnectButton = Cast<UButton>(MatchmakingWidget->GetWidgetFromName(TEXT("ConnectButton")))) {
            ConnectButton->OnClicked.AddDynamic(this, &AMatchmakingLevelScript::OnConnectClicked);
        }

        if (UButton* HostButton = Cast<UButton>(MatchmakingWidget->GetWidgetFromName(TEXT("HostButton")))) {
            HostButton->OnClicked.AddDynamic(this, &AMatchmakingLevelScript::OnHostClicked);
            HostButton->SetIsEnabled(false);
        }

        if (UButton* HostSteamButton = Cast<UButton>(MatchmakingWidget->GetWidgetFromName(TEXT("HostSteamButton")))) {
            HostSteamButton->OnClicked.AddDynamic(this, &AMatchmakingLevelScript::OnHostSteamClicked);
        }

        BackendIPTextBox = Cast<UEditableTextBox>(MatchmakingWidget->GetWidgetFromName(TEXT("BackendIPInput")));
        if (BackendIPTextBox && BackendSettingsSubsystem) {
            FString CurrentAddr = FString::Printf(TEXT("%s:%d"),
                *BackendSettingsSubsystem->GetBackendIP(),
                BackendSettingsSubsystem->GetBackendPort());
            BackendIPTextBox->SetText(FText::FromString(CurrentAddr));
        }

        if (UButton* ApplyIPButton = Cast<UButton>(MatchmakingWidget->GetWidgetFromName(TEXT("ApplyIPButton")))) {
            ApplyIPButton->OnClicked.AddDynamic(this, &AMatchmakingLevelScript::OnApplyBackendIPClicked);
        }

        ServerListScrollBoxWidget = Cast<UScrollBox>(MatchmakingWidget->GetWidgetFromName(TEXT("MyScrollBox")));
    }
}

void AMatchmakingLevelScript::EndPlay(const EEndPlayReason::Type EndPlayReason) {
    if (RefreshTimerHandle.IsValid()) {
        GetWorldTimerManager().ClearTimer(RefreshTimerHandle);
    }

    if (MatchSubsystem) {
        MatchSubsystem->OnSessionsUpdated.RemoveDynamic(this, &AMatchmakingLevelScript::OnSessionsUpdated);
        MatchSubsystem->OnHostRequested.RemoveDynamic(this, &AMatchmakingLevelScript::OnHostRequested);
        MatchSubsystem->OnConnectionStatusChanged.RemoveDynamic(this, &AMatchmakingLevelScript::OnConnectionStatusChanged);
        MatchSubsystem->OnJoinSuccess.RemoveDynamic(this, &AMatchmakingLevelScript::OnJoinSuccess);
        MatchSubsystem->OnServerError.RemoveDynamic(this, &AMatchmakingLevelScript::OnServerError);
    }

    SteamSessionMgr = nullptr;
    BackendSettingsSubsystem = nullptr;
    BackendIPTextBox = nullptr;

    Super::EndPlay(EndPlayReason);
}

void AMatchmakingLevelScript::RefreshSessionList() {
    if (MatchSubsystem) {
        UE_LOG(LogTemp, Log, TEXT("AMatchmakingLevelScript::RefreshSessionList - Timer triggered"));
        MatchSubsystem->RefreshSessionList();
    }
}

void AMatchmakingLevelScript::OnConnectClicked() {
    if (MatchSubsystem) {
        MatchSubsystem->ConnectToMatchmakingServer();
        if (UButton* HostButton = Cast<UButton>(MatchmakingWidget->GetWidgetFromName(TEXT("HostButton")))) {
            HostButton->SetIsEnabled(true);
        }
    }
}

void AMatchmakingLevelScript::OnHostClicked() {
    if (MatchSubsystem) {
        MatchSubsystem->HostNewGame(TEXT("My test server"));
    }
}

void AMatchmakingLevelScript::OnHostSteamClicked() {
    if (SteamSessionMgr) {
        UE_LOG(LogTemp, Log, TEXT("Creating Steam P2P session..."));

        SteamSessionMgr->CreateSteamSession(
            TEXT("Steam P2P Game"),
            8,
            TEXT("Default"),
            TEXT("/Game/ThirdPerson/Lvl_ThirdPerson"),
            false
        );
    }
    else {
        UE_LOG(LogTemp, Error, TEXT("SteamSessionManager not available!"));
    }
}

void AMatchmakingLevelScript::OnApplyBackendIPClicked() {
    if (!BackendSettingsSubsystem) {
        UE_LOG(LogTemp, Error, TEXT("BackendSettings not available!"));
        return;
    }

    if (BackendIPTextBox) {
        FString NewIP = BackendIPTextBox->GetText().ToString().TrimStartAndEnd();
        if (!NewIP.IsEmpty()) {
            FString IP;
            FString PortStr;
            if (NewIP.Split(TEXT(":"), &IP, &PortStr)) {
                int32 Port = FCString::Atoi(*PortStr);
                if (Port > 0) {
                    BackendSettingsSubsystem->SetBackendAddress(IP, Port);
                }
                else {
                    BackendSettingsSubsystem->SetBackendAddress(IP);
                }
            }
            else {
                BackendSettingsSubsystem->SetBackendAddress(NewIP);
            }
            BackendSettingsSubsystem->SaveSettings();
            UE_LOG(LogTemp, Log, TEXT("Backend address updated to: %s:%d"),
                *BackendSettingsSubsystem->GetBackendIP(),
                BackendSettingsSubsystem->GetBackendPort());
        }
    }
}

void AMatchmakingLevelScript::OnJoinSessionClicked(int32 SessionId) {
    if (!MatchSubsystem) return;

    const TArray<FMatchSessionInfo>& Sessions = MatchSubsystem->GetSessions();
    for (const FMatchSessionInfo& SI : Sessions) {
        if (SI.Id == SessionId) {
            if (SI.IsSteamP2P()) {
                UE_LOG(LogTemp, Log, TEXT("Joining Steam P2P session %d (Host: %s, SteamId: %s)"),
                    SessionId, *SI.HostPlayerName, *SI.HostSteamId);
                MatchSubsystem->JoinSessionByInfo(SI);
            }
            else {
                UE_LOG(LogTemp, Log, TEXT("Joining dedicated session %d (%s:%d)"),
                    SessionId, *SI.ServerIp, SI.ServerPort);
                MatchSubsystem->JoinSession(SessionId);
            }
            return;
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("Session %d not found in list, trying TCP join"), SessionId);
    MatchSubsystem->JoinSession(SessionId);
}

void AMatchmakingLevelScript::OnSessionsUpdated(const TArray<FMatchSessionInfo>& Sessions) {
    UE_LOG(LogTemp, Log, TEXT("Sessions updated - Count: %d"), Sessions.Num());
    RebuildServerListUI();
}

void AMatchmakingLevelScript::OnHostRequested(int32 SessionId, FString ServerIp, int32 ServerPort) {
    UE_LOG(LogTemp, Warning, TEXT("Host request confirmed - SessionId: %d, IP: %s, Port: %d"),
        SessionId, *ServerIp, ServerPort);

    if (APlayerController* PC = GetWorld()->GetFirstPlayerController()) {
        FString Cmd = FString::Printf(TEXT("open %s:%d"), *ServerIp, ServerPort);
        PC->ConsoleCommand(*Cmd);
    }
}

void AMatchmakingLevelScript::OnConnectionStatusChanged(bool bIsConnected) {
    UE_LOG(LogTemp, Warning, TEXT("AMatchmakingLevelScript::OnConnectionStatusChanged - Connected: %s"),
        bIsConnected ? TEXT("YES") : TEXT("NO"));

    if (bIsConnected) {
        if (!RefreshTimerHandle.IsValid()) {
            UE_LOG(LogTemp, Log, TEXT("Starting session refresh timer with interval: %.2f seconds"), RefreshInterval);
            GetWorldTimerManager().SetTimer(
                RefreshTimerHandle,
                this,
                &AMatchmakingLevelScript::RefreshSessionList,
                RefreshInterval,
                true
            );
        }
    }
    else {
        if (RefreshTimerHandle.IsValid()) {
            UE_LOG(LogTemp, Warning, TEXT("Stopping session refresh timer - disconnected"));
            GetWorldTimerManager().ClearTimer(RefreshTimerHandle);
            RefreshTimerHandle.Invalidate();
        }
    }
}

void AMatchmakingLevelScript::OnJoinSuccess(int32 SessionId, FString ServerIp, int32 ServerPort) {
    UE_LOG(LogTemp, Warning, TEXT("Join confirmed - SessionId: %d, IP: %s, Port: %d"),
        SessionId, *ServerIp, ServerPort);

    if (APlayerController* PC = GetWorld()->GetFirstPlayerController()) {
        FString Cmd = FString::Printf(TEXT("open %s:%d"), *ServerIp, ServerPort);
        PC->ConsoleCommand(*Cmd);
    }
}

void AMatchmakingLevelScript::OnServerError(FString ErrorCode) {
    UE_LOG(LogTemp, Error, TEXT("Server error received: %s"), *ErrorCode);

    if (ErrorCode == TEXT("NoAvailablePorts")) {
        UE_LOG(LogTemp, Error, TEXT("Server has no available ports to host a new session!"));
    }
    else if (ErrorCode == TEXT("SessionNotFound")) {
        UE_LOG(LogTemp, Error, TEXT("The session you're trying to join no longer exists!"));
        RefreshSessionList();
    }
    else if (ErrorCode == TEXT("NotAuthorized")) {
        UE_LOG(LogTemp, Error, TEXT("You're not authorized to perform this action!"));
    }
}

void AMatchmakingLevelScript::RebuildServerListUI() {
    if (!ServerListScrollBoxWidget || !MatchSubsystem) return;

    TArray<UWidget*> children = ServerListScrollBoxWidget->GetAllChildren();
    for (UWidget* W : children) {
        W->RemoveFromParent();
    }

    const TArray<FMatchSessionInfo>& Sessions = MatchSubsystem->GetSessions();

    if (Sessions.Num() == 0) {
        UVerticalBox* Box = NewObject<UVerticalBox>(this);
        ServerListScrollBoxWidget->AddChild(Box);

        UTextBlock* EmptyText = NewObject<UTextBlock>(this);
        EmptyText->SetText(FText::FromString(TEXT("No sessions available")));
        Box->AddChildToVerticalBox(EmptyText);
        return;
    }

    for (const FMatchSessionInfo& SI : Sessions) {
        UVerticalBox* Box = NewObject<UVerticalBox>(this);
        ServerListScrollBoxWidget->AddChild(Box);

        UServerButton* ItemBtn = NewObject<UServerButton>(this);
        ItemBtn->SetSessionInfo(SI);

        FString DisplayText;
        if (SI.IsSteamP2P()) {
            DisplayText = FString::Printf(TEXT("[P2P] %s (%d/%d) - Host: %s | %s"),
                *SI.Name, SI.PlayerCount, SI.MaxPlayers, *SI.HostPlayerName, *SI.GameMode);
        }
        else {
            DisplayText = FString::Printf(TEXT("[Dedicated] %s (%d players) - %s:%d"),
                *SI.Name, SI.PlayerCount, *SI.ServerIp, SI.ServerPort);
        }

        UTextBlock* Txt = NewObject<UTextBlock>(this);
        Txt->SetText(FText::FromString(DisplayText));
        ItemBtn->AddChild(Txt);

        ItemBtn->OnServerButtonClicked.AddDynamic(this, &AMatchmakingLevelScript::OnJoinSessionClicked);
        Box->AddChildToVerticalBox(ItemBtn);
    }
}
