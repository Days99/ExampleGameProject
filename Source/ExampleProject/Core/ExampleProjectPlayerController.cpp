// Copyright Epic Games, Inc. All Rights Reserved.


#include "ExampleProjectPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Blueprint/UserWidget.h"
#include "../ExampleProject.h"
#include "VoiceChatComponent.h"
#include "Widgets/Input/SVirtualJoystick.h"

void AExampleProjectPlayerController::BeginPlay()
{
	Super::BeginPlay();

}

void AExampleProjectPlayerController::InitializeVoiceChat()
{
	if (IsLocalPlayerController() && !HasAuthority())
	{
		VoiceComp = NewObject<UVoiceChatComponent>(this);
		VoiceComp->RegisterComponent();
		VoiceComp->StartVoiceChat();
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

