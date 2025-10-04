// Fill out your copyright notice in the Description page of Project Settings.


#include "CoinsGameStateBase.h"
#include <Kismet/GameplayStatics.h>
#include <Net/UnrealNetwork.h>
#include "../Collectibles/CoinActor.h"


ACoinsGameStateBase::ACoinsGameStateBase()
{
	UpdateTotalCoinsInLevel();
}


void ACoinsGameStateBase::UpdateTotalCoinsInLevel()
{
	TArray<AActor*> FoundCoins;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACoinActor::StaticClass(), FoundCoins);
	TotalLevelCoins = FoundCoins.Num();
	UE_LOG(LogTemp, Warning, TEXT("Update Total Coins"));
}

void ACoinsGameStateBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ACoinsGameStateBase, TotalLevelCoins);
}

void ACoinsGameStateBase::MulticastOnLevelComplete_Implementation(APawn* character, bool succeeded)
{
	OnLevelCompleted(character, succeeded);
}
