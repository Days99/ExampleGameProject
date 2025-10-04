// Fill out your copyright notice in the Description page of Project Settings.


#include "CoinActor.h"
#include "GameFramework/Character.h"
#include "ExampleProjectCharacter.h"
#include <Kismet/GameplayStatics.h>
#include <CoinsGameStateBase.h>

// Sets default values
ACoinActor::ACoinActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;


	// Create root component
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

	// Create collision sphere
	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->SetupAttachment(RootComponent);
	CollisionSphere->SetSphereRadius(100.0f);

	CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &ACoinActor::OnOverlapBegin);

	SetReplicates(true);
}

// Called when the game starts or when spawned
void ACoinActor::BeginPlay()
{
	Super::BeginPlay();
	
}

void ACoinActor::PlayEffects_Implementation()
{ 
	UGameplayStatics::SpawnEmitterAtLocation(this, CollectEffects, GetActorLocation()); 
}

// Called every frame
void ACoinActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}


void ACoinActor::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// Check if the overlapping actor is a character
	if (ACharacter* Character = Cast<ACharacter>(OtherActor))
	{
		if (AExampleProjectCharacter* mycharacter = Cast<AExampleProjectCharacter>(Character))
		{
			// Debug logging to identify network role
			FString NetworkRoleString;
			switch (GetLocalRole())
			{
			case ROLE_Authority:
				NetworkRoleString = TEXT("Server (Authority)");
				break;
			case ROLE_AutonomousProxy:
				NetworkRoleString = TEXT("Client (Autonomous Proxy)");
				break;
			case ROLE_SimulatedProxy:
				NetworkRoleString = TEXT("Client (Simulated Proxy)");
				break;
			default:
				NetworkRoleString = TEXT("Unknown Role");
				break;
			}
			
			UE_LOG(LogTemp, Warning, TEXT("CoinActor::OnOverlapBegin - Running on: %s"), *NetworkRoleString);
			
			if (HasAuthority())
			{
				mycharacter->CollectCoin();
				PlayEffects();	
				Destroy();

				ACoinsGameStateBase* gamestate = Cast<ACoinsGameStateBase>(GetWorld()->GetGameState());
				if (gamestate != nullptr)
				{
					gamestate->UpdateTotalCoinsInLevel();
				}
			}


		}
	}
}

