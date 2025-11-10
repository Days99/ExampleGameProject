// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PhysicsReplicatedActor.generated.h"

/**
 * An actor with a mesh that simulates physics on the server and replicates to clients
 */
UCLASS()
class EXAMPLEPROJECT_API APhysicsReplicatedActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APhysicsReplicatedActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	/** The static mesh component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UStaticMeshComponent* MeshComponent;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	/** Set the mesh for this actor */
	UFUNCTION(BlueprintCallable, Category = "Physics Actor")
	void SetMesh(UStaticMesh* NewMesh);

	/** Apply an impulse to the mesh (only works on server) */
	UFUNCTION(BlueprintCallable, Category = "Physics Actor")
	void ApplyImpulse(FVector Impulse);

	/** Apply force to the mesh (only works on server) */
	UFUNCTION(BlueprintCallable, Category = "Physics Actor")
	void ApplyForce(FVector Force);

	// ===== OWNERSHIP DEMONSTRATION METHODS =====

	/** 
	 * Server RPC to demonstrate ownership.
	 * This will only execute if called by the owning client or the server.
	 */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Ownership Demo")
	void ServerDemoMethod(const FString& CallerIdentification);

	/**
	 * Test method to call the Server RPC.
	 * Call this from anywhere to see if the RPC succeeds based on ownership.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ownership Demo")
	void TestCallServerMethod(const FString& TestMessage);

	/**
	 * Set ownership of this actor to a specific player controller.
	 * Pass nullptr to set ownership back to the server.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ownership Demo")
	void SetOwnershipToPlayer(APlayerController* NewOwner);

	/**
	 * Display current ownership information.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ownership Demo")
	void DisplayOwnershipInfo();
};


