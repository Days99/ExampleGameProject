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
            MatchSubsystem->OnSessionsUpdated.AddDynamic(this, &AMatchmakingLevelScript::OnSessionsUpdated);
            MatchSubsystem->OnHostRequested.AddDynamic(this, &AMatchmakingLevelScript::OnHostRequested);
            MatchSubsystem->OnConnectionStatusChanged.AddDynamic(this, &AMatchmakingLevelScript::OnConnectionStatusChanged);
        }
    }

    // Create widget and connect buttons like before, but button callbacks call into subsystem
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
        // Example: instruct subsystem to host on port 7777 with name
        MatchSubsystem->HostNewGame(TEXT("My test server"), 7777);
    }
}

void AMatchmakingLevelScript::OnSessionsUpdated(const TArray<FMatchSessionInfo>& Sessions) {
    // Rebuild UI using Sessions array (safely on game thread)
    RebuildServerListUI();
}

void AMatchmakingLevelScript::OnHostRequested(FString IpAdress, int32 Port) {
    // Called when server accepted our host request
    if (APlayerController* PC = GetWorld()->GetFirstPlayerController()) {
        FString Cmd = FString::Printf(TEXT("open %s:%d"), *IpAdress, Port);
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

void AMatchmakingLevelScript::RebuildServerListUI() {
    if (!ServerListScrollBoxWidget || !MatchSubsystem) return;

    // Clear existing
    TArray<UWidget*> children = ServerListScrollBoxWidget->GetAllChildren();
    for (UWidget* W : children) {
        W->RemoveFromParent();
    }

    // Fill with current sessions
    const TArray<FMatchSessionInfo>& Sessions = MatchSubsystem->GetSessions();
    for (const FMatchSessionInfo& SI : Sessions) {
        UVerticalBox* Box = NewObject<UVerticalBox>(this);
        ServerListScrollBoxWidget->AddChild(Box);

        // Create a simple button widget for each entry (you probably have a UMyButton class)
        UServerButton* ItemBtn = NewObject<UServerButton>(this);
        ItemBtn->SetSessionInfo(SI);
        UTextBlock* Txt = NewObject<UTextBlock>(this);
        Txt->SetText(FText::FromString(SI.Name));
        ItemBtn->AddChild(Txt);
        Box->AddChildToVerticalBox(ItemBtn);
    }
}
