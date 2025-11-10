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

	// Display ownership info for debugging
	DisplayOwnershipInfo();
	
	// Only simulate physics on the server (authority)
	if (HasAuthority())
	{
		MeshComponent->SetSimulatePhysics(true);
		UE_LOG(LogTemp, Log, TEXT("PhysicsReplicatedActor %s: Physics enabled on SERVER"), *GetName());
	}
	else
	{

		
		//SetOwnershipToPlayer(GetWorld()->GetFirstPlayerController());
		//TestCallServerMethod("Called on Client");
		// On clients, disable physics simulation - just receive replicated transforms
		MeshComponent->SetSimulatePhysics(false);
		UE_LOG(LogTemp, Log, TEXT("PhysicsReplicatedActor %s: Physics DISABLED on CLIENT (receiving replication)"), *GetName());
		
		// NOTE: Don't call TestCallServerMethod here! It runs before ownership replicates.
		// Instead, call it manually after a delay (e.g., from a Blueprint timer or input binding)
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

// ===== OWNERSHIP DEMONSTRATION METHODS IMPLEMENTATION =====

void APhysicsReplicatedActor::ServerDemoMethod_Implementation(const FString& CallerIdentification)
{
	// This method will ONLY execute on the server if:
	// 1. Called from the server itself, OR
	// 2. Called from the client that OWNS this actor
	
	UE_LOG(LogTemp, Warning, TEXT("========================================"));
	UE_LOG(LogTemp, Warning, TEXT("SERVER RPC EXECUTED SUCCESSFULLY!"));
	UE_LOG(LogTemp, Warning, TEXT("Actor: %s"), *GetName());
	UE_LOG(LogTemp, Warning, TEXT("Message: %s"), *CallerIdentification);
	UE_LOG(LogTemp, Warning, TEXT("Owner: %s"), GetOwner() ? *GetOwner()->GetName() : TEXT("SERVER (no owner set)"));
	UE_LOG(LogTemp, Warning, TEXT("========================================"));
}

void APhysicsReplicatedActor::TestCallServerMethod(const FString& TestMessage)
{
	// Log where this is being called from
	FString NetRoleStr = TEXT("UNKNOWN");
	if (HasAuthority())
	{
		NetRoleStr = TEXT("SERVER");
	}
	else
	{
		NetRoleStr = TEXT("CLIENT");
	}

	FString OwnerStr = GetOwner() ? GetOwner()->GetName() : TEXT("SERVER (no owner)");
	
	UE_LOG(LogTemp, Display, TEXT("----------------------------------------"));
	UE_LOG(LogTemp, Display, TEXT("TestCallServerMethod called on %s"), *NetRoleStr);
	UE_LOG(LogTemp, Display, TEXT("Actor: %s"), *GetName());
	UE_LOG(LogTemp, Display, TEXT("Owner: %s"), *OwnerStr);
	UE_LOG(LogTemp, Display, TEXT("Test Message: %s"), *TestMessage);
	UE_LOG(LogTemp, Display, TEXT("Attempting to call Server RPC..."));
	UE_LOG(LogTemp, Display, TEXT("----------------------------------------"));

	// Attempt to call the Server RPC
	// If this actor is owned by the server and called from a client, this will FAIL silently
	// If this actor is owned by the calling client, this will SUCCEED
	// If called from the server, this will always SUCCEED
	ServerDemoMethod(TestMessage);
	
	// This log always appears, but the Server RPC might not execute
	UE_LOG(LogTemp, Display, TEXT("Client-side: Server RPC call dispatched (check server logs to see if it executed)"));
}

void APhysicsReplicatedActor::SetOwnershipToPlayer(APlayerController* NewOwner)
{
	if (!HasAuthority())
	{
		UE_LOG(LogTemp, Error, TEXT("SetOwnershipToPlayer can only be called on the SERVER!"));
		return;
	}

	// Set the owner
	SetOwner(NewOwner);
	
	// This ensures the actor starts replicating immediately
	FlushNetDormancy();
	// This ensures the ownership change is immediately replicated
	ForceNetUpdate();
	
	if (NewOwner)
	{
		UE_LOG(LogTemp, Warning, TEXT("Actor %s ownership set to PlayerController: %s"), 
			*GetName(), *NewOwner->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Actor %s ownership set to SERVER (owner = nullptr)"), 
			*GetName());
	}

	// Display current ownership info
	DisplayOwnershipInfo();
}

void APhysicsReplicatedActor::DisplayOwnershipInfo()
{
	FString NetRoleStr;
	switch (GetLocalRole())
	{
		case ROLE_Authority:
			NetRoleStr = TEXT("AUTHORITY (Server)");
			break;
		case ROLE_AutonomousProxy:
			NetRoleStr = TEXT("AUTONOMOUS PROXY (Owned by local client)");
			break;
		case ROLE_SimulatedProxy:
			NetRoleStr = TEXT("SIMULATED PROXY (Not owned by local client)");
			break;
		default:
			NetRoleStr = TEXT("NONE");
	}

	FString OwnerStr = GetOwner() ? GetOwner()->GetName() : TEXT("SERVER (no owner set)");
	
	UE_LOG(LogTemp, Display, TEXT("========================================"));
	UE_LOG(LogTemp, Display, TEXT("OWNERSHIP INFO for %s"), *GetName());
	UE_LOG(LogTemp, Display, TEXT("Local Role: %s"), *NetRoleStr);
	UE_LOG(LogTemp, Display, TEXT("Owner: %s"), *OwnerStr);
	UE_LOG(LogTemp, Display, TEXT("Has Authority: %s"), HasAuthority() ? TEXT("YES") : TEXT("NO"));
	UE_LOG(LogTemp, Display, TEXT("========================================"));
}


