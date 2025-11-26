// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/ServerButton.h"

UServerButton::UServerButton()
{
    // Bind internal click handler
    OnClicked.AddDynamic(this, &UServerButton::OnClick);
}

void UServerButton::SetSessionInfo(const FMatchSessionInfo& InInfo) {
    SessionInfo = InInfo;
}

void UServerButton::OnClick() {
    // Simply broadcast the session ID - let the level script handle the join logic
    UE_LOG(LogTemp, Log, TEXT("Server button clicked - SessionId: %d, Name: %s"),
        SessionInfo.Id, *SessionInfo.Name);

    OnServerButtonClicked.Broadcast(SessionInfo.Id);
}

