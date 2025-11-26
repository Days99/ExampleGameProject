// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/Button.h"
#include "Server/MatchSessionInfo.h"
#include "ServerButton.generated.h"

// Delegate for when server button is clicked
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnServerButtonClicked, int32, SessionId);

/**
 * Button widget for displaying and selecting a game session
 */
UCLASS()
class UServerButton : public UButton
{
    GENERATED_BODY()

public:
    UServerButton();

    // Set the session info for this button
    void SetSessionInfo(const FMatchSessionInfo& InInfo);

    // Get the session info
    const FMatchSessionInfo& GetSessionInfo() const { return SessionInfo; }

    // Delegate that fires when button is clicked, passes the session ID
    UPROPERTY(BlueprintAssignable, Category = "ServerButton")
    FOnServerButtonClicked OnServerButtonClicked;

protected:
    UFUNCTION()
    void OnClick();

private:
    FMatchSessionInfo SessionInfo;
};

