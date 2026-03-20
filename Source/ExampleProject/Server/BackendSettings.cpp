#include "Server/BackendSettings.h"
#include "Misc/ConfigCacheIni.h"

void UBackendSettings::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    LoadSettings();

    UE_LOG(LogTemp, Log, TEXT("BackendSettings initialized - Server: %s:%d"),
        *BackendIP, BackendPort);
}

void UBackendSettings::SetBackendIP(const FString& NewIP)
{
    BackendIP = NewIP;
    UE_LOG(LogTemp, Log, TEXT("Backend IP set to: %s"), *BackendIP);
}

void UBackendSettings::SetBackendPort(int32 NewPort)
{
    BackendPort = NewPort;
    UE_LOG(LogTemp, Log, TEXT("Backend Port set to: %d"), BackendPort);
}

void UBackendSettings::SetBackendAddress(const FString& IP, int32 Port)
{
    SetBackendIP(IP);
    SetBackendPort(Port);
    UE_LOG(LogTemp, Log, TEXT("Backend address configured - %s:%d"), *BackendIP, BackendPort);
}

void UBackendSettings::SaveSettings()
{
    GConfig->SetString(TEXT("BackendSettings"), TEXT("BackendIP"), *BackendIP, GGameIni);
    GConfig->SetInt(TEXT("BackendSettings"), TEXT("BackendPort"), BackendPort, GGameIni);
    GConfig->Flush(false, GGameIni);

    UE_LOG(LogTemp, Log, TEXT("Backend settings saved to config"));
}

void UBackendSettings::LoadSettings()
{
    FString LoadedIP;
    if (GConfig->GetString(TEXT("BackendSettings"), TEXT("BackendIP"), LoadedIP, GGameIni))
    {
        BackendIP = LoadedIP;
    }

    int32 LoadedPort;
    if (GConfig->GetInt(TEXT("BackendSettings"), TEXT("BackendPort"), LoadedPort, GGameIni))
    {
        BackendPort = LoadedPort;
    }

    UE_LOG(LogTemp, Log, TEXT("Backend settings loaded - Server: %s:%d"),
        *BackendIP, BackendPort);
}
