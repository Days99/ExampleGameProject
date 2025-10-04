// Fill out your copyright notice in the Description page of Project Settings.


#include "LevelCompleteArea.h"
#include <Components/BoxComponent.h>
#include "../Core/ExampleProjectCharacter.h"
#include "../Core/CoinsGameStateBase.h"

// Sets default values
ALevelCompleteArea::ALevelCompleteArea()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	BoxComponent = CreateDefaultSubobject<UBoxComponent>("BoxComponent");
	BoxComponent->SetBoxExtent(FVector(200.0f, 200.0f, 200.0f));
	BoxComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	BoxComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	BoxComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	BoxComponent->SetupAttachment(RootComponent);
	BoxComponent->OnComponentBeginOverlap.AddDynamic(this, &ALevelCompleteArea::OnOverlapBegin);
}

// Called when the game starts or when spawned
void ALevelCompleteArea::BeginPlay()
{
	Super::BeginPlay();
	
}

void ALevelCompleteArea::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32
	OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (HasAuthority())
	{
		AExampleProjectCharacter* Character = Cast<AExampleProjectCharacter>(OtherActor);
		if (Character != nullptr)
		{
			ACoinsGameStateBase* GameState = Cast<ACoinsGameStateBase>(GetWorld()->GetGameState());
			if (GameState != nullptr)
			{
				if (GameState->TotalLevelCoins == 0)
					GameState->MulticastOnLevelComplete(Character, true);
			}
		}
	}
}

// Called every frame
void ALevelCompleteArea::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

