// Copyright Epic Games, Inc. All Rights Reserved.


#include "ExampleProjectPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Blueprint/UserWidget.h"
#include "../ExampleProject.h"
#include "VoiceChatComponent.h"
#include "Widgets/Input/SVirtualJoystick.h"
#include "Server/VoiceRoomManager.h"
#include "ExampleProjectGameMode.h"

void AExampleProjectPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (IsLocalPlayerController())
	{
		VoiceComp = NewObject<UVoiceChatComponent>(this);
		VoiceComp->RegisterComponent();
		VoiceComp->Initialize();
	}

	// only spawn touch controls on local player controllers
	if (IsLocalPlayerController() && SVirtualJoystick::ShouldDisplayTouchInterface())
	{
		// spawn the mobile controls widget
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

		if (MobileControlsWidget)
		{
			// add the controls to the player screen
			MobileControlsWidget->AddToPlayerScreen(0);

		}
		else {

			UE_LOG(LogExampleProject, Error, TEXT("Could not spawn mobile controls widget."));

		}

	}

}

void AExampleProjectPlayerController::InitializeVoiceChat()
{
	if (IsLocalPlayerController())
	{
		VoiceComp = NewObject<UVoiceChatComponent>(this);
		VoiceComp->RegisterComponent();
		VoiceComp->Initialize();
	}


}

void AExampleProjectPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// only add IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		// Add Input Mapping Contexts
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

			// only add these IMCs if we're not using mobile touch input
			if (!SVirtualJoystick::ShouldDisplayTouchInterface())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}
		}
	}
}

bool AExampleProjectPlayerController::Server_RequestVoiceCredentials_Validate(const FString& ProductUserId)
{
	return !ProductUserId.IsEmpty();
}

void AExampleProjectPlayerController::Server_RequestVoiceCredentials_Implementation(const FString& ProductUserId)
{
	UE_LOG(LogTemp, Log, TEXT("[PlayerController] Server received voice credential request from %s"), *ProductUserId);
	
	// Get GameMode and request credentials
	if (AExampleProjectGameMode* GameMode = GetWorld()->GetAuthGameMode<AExampleProjectGameMode>())
	{
		GameMode->RequestVoiceCredentialsForPlayer(this, ProductUserId);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[PlayerController] GameMode not found or wrong type"));
	}
}

void AExampleProjectPlayerController::Client_ReceiveVoiceCredentials_Implementation(const FString& ChannelName, const FString& Token)
{
	UE_LOG(LogTemp, Log, TEXT("[PlayerController] Client received voice credentials for channel: %s"), *ChannelName);
	
	// Join voice channel with server-provided credentials
	if (VoiceComp && VoiceComp->IsReady())
	{
		UE_LOG(LogTemp, Log, TEXT("[PlayerController] Voice ready, joining channel immediately"));
		VoiceComp->JoinChannel(ChannelName, Token, false);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[PlayerController] Voice component not ready, storing credentials for later"));
		// Store credentials and join when ready
		PendingChannelName = ChannelName;
		PendingToken = Token;
	}
}

