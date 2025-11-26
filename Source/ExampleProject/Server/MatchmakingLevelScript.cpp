// Fill out your copyright notice in the Description page of Project Settings.

#include "MatchmakingLevelScript.h"

#include "Blueprint/UserWidget.h"
#include "MatchmakingSubsystem.h"
#include "Core/ServerButton.h"
#include "Components/Button.h"
#include "Components/ScrollBox.h"
#include "Components/VerticalBox.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

void AMatchmakingLevelScript::BeginPlay() {
    Super::BeginPlay();

    // Enable tick for refresh timer
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;

    if (UGameInstance* GI = GetGameInstance()) {
        MatchSubsystem = GI->GetSubsystem<UMatchmakingSubsystem>();
        if (MatchSubsystem) {
            // Bind to subsystem delegates
            MatchSubsystem->OnSessionsUpdated.AddDynamic(this, &AMatchmakingLevelScript::OnSessionsUpdated);
            MatchSubsystem->OnHostRequested.AddDynamic(this, &AMatchmakingLevelScript::OnHostRequested);
            MatchSubsystem->OnConnectionStatusChanged.AddDynamic(this, &AMatchmakingLevelScript::OnConnectionStatusChanged);
            MatchSubsystem->OnJoinSuccess.AddDynamic(this, &AMatchmakingLevelScript::OnJoinSuccess);
            MatchSubsystem->OnServerError.AddDynamic(this, &AMatchmakingLevelScript::OnServerError);
        }
    }

    // Create widget and connect buttons
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

        ServerListScrollBoxWidget = Cast<UScrollBox>(MatchmakingWidget->GetWidgetFromName(TEXT("MyScrollBox")));
    }
}

void AMatchmakingLevelScript::EndPlay(const EEndPlayReason::Type EndPlayReason) {
    // Clear the timer
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
        // FIXED: HostNewGame now only takes session name (no port parameter)
        MatchSubsystem->HostNewGame(TEXT("My test server"));
    }
}

void AMatchmakingLevelScript::OnJoinSessionClicked(int32 SessionId) {
    if (MatchSubsystem) {
        UE_LOG(LogTemp, Log, TEXT("Attempting to join session %d"), SessionId);
        MatchSubsystem->JoinSession(SessionId);
    }
}

void AMatchmakingLevelScript::OnSessionsUpdated(const TArray<FMatchSessionInfo>& Sessions) {
    // Rebuild UI using Sessions array (safely on game thread)
    UE_LOG(LogTemp, Log, TEXT("Sessions updated - Count: %d"), Sessions.Num());
    RebuildServerListUI();
}

void AMatchmakingLevelScript::OnHostRequested(int32 SessionId, FString ServerIp, int32 ServerPort) {
    // FIXED: Updated signature to match new delegate (SessionId, ServerIp, ServerPort)
    UE_LOG(LogTemp, Warning, TEXT("Host request confirmed - SessionId: %d, IP: %s, Port: %d"),
        SessionId, *ServerIp, ServerPort);

    // Travel to the hosted server
    if (APlayerController* PC = GetWorld()->GetFirstPlayerController()) {
        FString Cmd = FString::Printf(TEXT("open %s:%d"), *ServerIp, ServerPort);
        PC->ConsoleCommand(*Cmd);
    }
}

void AMatchmakingLevelScript::OnConnectionStatusChanged(bool bIsConnected) {
    UE_LOG(LogTemp, Warning, TEXT("AMatchmakingLevelScript::OnConnectionStatusChanged - Connected: %s"),
        bIsConnected ? TEXT("YES") : TEXT("NO"));

    if (bIsConnected) {
        // Start the refresh timer now that we're connected
        if (!RefreshTimerHandle.IsValid()) {
            UE_LOG(LogTemp, Log, TEXT("Starting session refresh timer with interval: %.2f seconds"), RefreshInterval);
            GetWorldTimerManager().SetTimer(
                RefreshTimerHandle,
                this,
                &AMatchmakingLevelScript::RefreshSessionList,
                RefreshInterval,
                true  // Loop
            );
        }
    }
    else {
        // Stop the refresh timer if we're disconnected
        if (RefreshTimerHandle.IsValid()) {
            UE_LOG(LogTemp, Warning, TEXT("Stopping session refresh timer - disconnected"));
            GetWorldTimerManager().ClearTimer(RefreshTimerHandle);
            RefreshTimerHandle.Invalidate();
        }
    }
}

void AMatchmakingLevelScript::OnJoinSuccess(int32 SessionId, FString ServerIp, int32 ServerPort) {
    // Called when server confirmed we successfully joined a session
    UE_LOG(LogTemp, Warning, TEXT("Join confirmed - SessionId: %d, IP: %s, Port: %d"),
        SessionId, *ServerIp, ServerPort);

    // Travel to the game server
    if (APlayerController* PC = GetWorld()->GetFirstPlayerController()) {
        FString Cmd = FString::Printf(TEXT("open %s:%d"), *ServerIp, ServerPort);
        PC->ConsoleCommand(*Cmd);
    }
}

void AMatchmakingLevelScript::OnServerError(FString ErrorCode) {
    // Handle server errors
    UE_LOG(LogTemp, Error, TEXT("Server error received: %s"), *ErrorCode);

    // You could display this in UI
    if (ErrorCode == TEXT("NoAvailablePorts")) {
        UE_LOG(LogTemp, Error, TEXT("Server has no available ports to host a new session!"));
        // Show error message to user in UI
    }
    else if (ErrorCode == TEXT("SessionNotFound")) {
        UE_LOG(LogTemp, Error, TEXT("The session you're trying to join no longer exists!"));
        // Refresh session list
        RefreshSessionList();
    }
    else if (ErrorCode == TEXT("NotAuthorized")) {
        UE_LOG(LogTemp, Error, TEXT("You're not authorized to perform this action!"));
    }
}

void AMatchmakingLevelScript::RebuildServerListUI() {
    if (!ServerListScrollBoxWidget || !MatchSubsystem) return;

    // Clear existing
    TArray<UWidget*> children = ServerListScrollBoxWidget->GetAllChildren();
    for (UWidget* W : children) {
        W->RemoveFromParent();
    }

    // Fill with current sessions
    const TArray<FMatchSessionInfo>& Sessions = MatchSubsystem->GetSessions();

    if (Sessions.Num() == 0) {
        // Optionally show "No sessions available" message
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

        // Create a server button widget for each entry
        UServerButton* ItemBtn = NewObject<UServerButton>(this);
        ItemBtn->SetSessionInfo(SI);

        // Display session info with player count
        FString DisplayText = FString::Printf(TEXT("%s (%d players) - %s:%d"),
            *SI.Name, SI.PlayerCount, *SI.ServerIp, SI.ServerPort);

        UTextBlock* Txt = NewObject<UTextBlock>(this);
        Txt->SetText(FText::FromString(DisplayText));
        ItemBtn->AddChild(Txt);

        // Bind click event to join this session
        ItemBtn->OnServerButtonClicked.AddDynamic(this, &AMatchmakingLevelScript::OnJoinSessionClicked);

        Box->AddChildToVerticalBox(ItemBtn);
    }
}

