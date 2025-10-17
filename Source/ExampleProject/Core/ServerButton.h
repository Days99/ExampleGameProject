// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/Button.h"
#include "Server/MatchSessionInfo.h"
#include "ServerButton.generated.h"

/**
 * 
 */
UCLASS()
class EXAMPLEPROJECT_API UServerButton : public UButton
{
	GENERATED_BODY()
public:  
	UServerButton();

    void SetSessionInfo(const FMatchSessionInfo& InInfo);

private:
    FMatchSessionInfo SessionInfo;
    UFUNCTION()
    void OnClick();
};
