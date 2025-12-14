// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ExampleProjectPlayerController.generated.h"

class UInputMappingContext;
class UUserWidget;
class UVoiceChatComponent;

/**
 *  Basic PlayerController class for a third person game
 *  Manages input mappings
 */
UCLASS(abstract)
class AExampleProjectPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	void InitializeVoiceChat();

	/** Server RPC: Request voice credentials for main channel */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_RequestVoiceCredentials(const FString& ProductUserId);

	/** Client RPC: Receive voice credentials from server */
	UFUNCTION(Client, Reliable)
	void Client_ReceiveVoiceCredentials(const FString& ChannelName, const FString& Token);
	
protected:

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category ="Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> MobileExcludedMappingContexts;

	/** Mobile controls widget to spawn */
	UPROPERTY(EditAnywhere, Category="Input|Touch Controls")
	TSubclassOf<UUserWidget> MobileControlsWidgetClass;

	/** Pointer to the mobile controls widget */
	TObjectPtr<UUserWidget> MobileControlsWidget;

	UVoiceChatComponent* VoiceComp;

public:
	/** Pending voice credentials (if received before voice is ready) */
	FString PendingChannelName;
	FString PendingToken;



	/** Gameplay initialization */
	virtual void BeginPlay() override;

	/** Input mapping context setup */
	virtual void SetupInputComponent() override;

};
