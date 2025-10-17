// Fill out your copyright notice in the Description page of Project Settings.

#include "PhysicsReplicatedActor.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"

// Sets default values
APhysicsReplicatedActor::APhysicsReplicatedActor()
{
 	// Set this actor to call Tick() every frame
	PrimaryActorTick.bCanEverTick = true;

	// Enable replication
	bReplicates = true;
	SetReplicateMovement(true);

	// Create the static mesh component
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	RootComponent = MeshComponent;

	// Configure mesh component
	MeshComponent->SetIsReplicated(true);
	
	// Enable collision
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	MeshComponent->SetCollisionProfileName(TEXT("PhysicsActor"));
	
	// Physics will be enabled in BeginPlay only on server
	MeshComponent->SetSimulatePhysics(false);
	
	// Enable generate overlap events if needed
	MeshComponent->SetGenerateOverlapEvents(true);
}

// Called when the game starts or when spawned
void APhysicsReplicatedActor::BeginPlay()
{
	Super::BeginPlay();
	
	// Only simulate physics on the server (authority)
	if (HasAuthority())
	{
		MeshComponent->SetSimulatePhysics(true);
		UE_LOG(LogTemp, Log, TEXT("PhysicsReplicatedActor %s: Physics enabled on SERVER"), *GetName());
	}
	else
	{
		// On clients, disable physics simulation - just receive replicated transforms
		MeshComponent->SetSimulatePhysics(false);
		UE_LOG(LogTemp, Log, TEXT("PhysicsReplicatedActor %s: Physics DISABLED on CLIENT (receiving replication)"), *GetName());
	}
}

// Called every frame
void APhysicsReplicatedActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void APhysicsReplicatedActor::SetMesh(UStaticMesh* NewMesh)
{
	if (MeshComponent && NewMesh)
	{
		MeshComponent->SetStaticMesh(NewMesh);
	}
}

void APhysicsReplicatedActor::ApplyImpulse(FVector Impulse)
{
	// Only apply impulse on server
	if (HasAuthority() && MeshComponent)
	{
		MeshComponent->AddImpulse(Impulse, NAME_None, true);
		UE_LOG(LogTemp, Log, TEXT("PhysicsReplicatedActor %s: Applied impulse %s"), *GetName(), *Impulse.ToString());
	}
}

void APhysicsReplicatedActor::ApplyForce(FVector Force)
{
	// Only apply force on server
	if (HasAuthority() && MeshComponent)
	{
		MeshComponent->AddForce(Force, NAME_None, true);
		UE_LOG(LogTemp, Log, TEXT("PhysicsReplicatedActor %s: Applied force %s"), *GetName(), *Force.ToString());
	}
}


