#pragma once

#include "CoreMinimal.h"
#include "Engine/LevelScriptActor.h"
#include "MatchSessionInfo.h"
#include "MatchmakingLevelScript.generated.h"

class UUserWidget;
class UMatchmakingSubsystem;
class USteamSessionManager;
class UBackendSettings;
class UScrollBox;
class UEditableTextBox;

/**
 * Level script for managing matchmaking UI and interactions
 */
UCLASS()
class AMatchmakingLevelScript : public ALevelScriptActor
{
    GENERATED_BODY()

public:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Matchmaking")
    TSubclassOf<UUserWidget> MatchmakingWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Matchmaking")
    float RefreshInterval = 5.0f;

protected:
    UFUNCTION()
    void OnConnectClicked();

    UFUNCTION()
    void OnHostClicked();

    /** Host a Steam P2P session */
    UFUNCTION()
    void OnHostSteamClicked();

    /** Apply the backend IP entered in the text box */
    UFUNCTION()
    void OnApplyBackendIPClicked();

    UFUNCTION()
    void OnJoinSessionClicked(int32 SessionId);

    UFUNCTION()
    void OnSessionsUpdated(const TArray<FMatchSessionInfo>& Sessions);

    UFUNCTION()
    void OnHostRequested(int32 SessionId, FString ServerIp, int32 ServerPort);

    UFUNCTION()
    void OnConnectionStatusChanged(bool bIsConnected);

    UFUNCTION()
    void OnJoinSuccess(int32 SessionId, FString ServerIp, int32 ServerPort);

    UFUNCTION()
    void OnServerError(FString ErrorCode);

    UFUNCTION()
    void RefreshSessionList();

    void RebuildServerListUI();

private:
    UPROPERTY()
    UUserWidget* MatchmakingWidget;

    UPROPERTY()
    UMatchmakingSubsystem* MatchSubsystem;

    UPROPERTY()
    USteamSessionManager* SteamSessionMgr;

    UPROPERTY()
    UBackendSettings* BackendSettingsSubsystem;

    UPROPERTY()
    UScrollBox* ServerListScrollBoxWidget;

    UPROPERTY()
    UEditableTextBox* BackendIPTextBox;

    FTimerHandle RefreshTimerHandle;
};

