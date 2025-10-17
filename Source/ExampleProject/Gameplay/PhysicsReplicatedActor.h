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
};


