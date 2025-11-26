// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MatchSessionInfo.generated.h"

USTRUCT(BlueprintType)
struct FMatchSessionInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Matchmaking")
    int32 Id = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Matchmaking")
    FString Name;

    UPROPERTY(BlueprintReadOnly, Category = "Matchmaking")
    FString ServerIp;

    UPROPERTY(BlueprintReadOnly, Category = "Matchmaking")
    int32 ServerPort = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Matchmaking")
    int32 PlayerCount = 0;
};

