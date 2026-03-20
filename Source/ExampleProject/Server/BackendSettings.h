#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "BackendSettings.generated.h"

/**
 * Game Instance Subsystem for managing backend server connection settings.
 * Allows runtime configuration of the matchmaking server IP address and port.
 * Settings persist via SaveConfig so they survive between sessions.
 */
UCLASS()
class EXAMPLEPROJECT_API UBackendSettings : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    /** Set the matchmaking server IP address */
    UFUNCTION(BlueprintCallable, Category = "Backend|Settings")
    void SetBackendIP(const FString& NewIP);

    /** Set the matchmaking server port */
    UFUNCTION(BlueprintCallable, Category = "Backend|Settings")
    void SetBackendPort(int32 NewPort);

    /** Get the matchmaking server IP address */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Backend|Settings")
    FString GetBackendIP() const { return BackendIP; }

    /** Get the matchmaking server port */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Backend|Settings")
    int32 GetBackendPort() const { return BackendPort; }

    /** Convenience: Set both IP and Port at once */
    UFUNCTION(BlueprintCallable, Category = "Backend|Settings")
    void SetBackendAddress(const FString& IP, int32 Port = 8856);

    /** Save settings to config file so they persist */
    UFUNCTION(BlueprintCallable, Category = "Backend|Settings")
    void SaveSettings();

    /** Load settings from config file */
    UFUNCTION(BlueprintCallable, Category = "Backend|Settings")
    void LoadSettings();

private:
    /** Matchmaking server IP address */
    FString BackendIP = TEXT("127.0.0.1");

    /** Matchmaking server port */
    int32 BackendPort = 8856;
};
