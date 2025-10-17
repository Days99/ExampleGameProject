#pragma once
#include "CoreMinimal.h"
#include "MatchSessionInfo.generated.h"

USTRUCT(BlueprintType)
struct FMatchSessionInfo {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    int32 Id = -1;

    UPROPERTY(BlueprintReadOnly)
    FString Name;

    UPROPERTY(BlueprintReadOnly)
    FString ServerIp;

    UPROPERTY(BlueprintReadOnly)
    int32 ServerPort = 0;
};
