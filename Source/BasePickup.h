// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/TimelineTemplate.h"
#include "CollectibleInterface.h"
#include "BasePickup.generated.h"

UCLASS()
class MYPROJECT_API ABasePickup : public AActor, public ICollectibleInterface
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ABasePickup();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Mesh component for visual representation
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* PickupMesh;

	// Collision component for overlap detection
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USphereComponent* CollisionSphere;

	// Resource value granted when collected
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup")
	int32 ResourceValue = 1;

	// Rotation speed for visual effect
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup")
	float RotationSpeed = 90.0f;

	// Bobbing amplitude
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup")
	float BobAmplitude = 50.0f;

	// Bobbing frequency
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup")
	float BobFrequency = 1.0f;

	// Timeline for bobbing animation
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup")
	UCurveFloat* BobCurve;

	// Timeline component (not exposed to Blueprint)
	FTimeline BobTimeline;

	// Initial Z position for bobbing
	float InitialZ;

	// Whether this pickup has been collected
	UPROPERTY(BlueprintReadOnly, Category = "Pickup")
	bool bIsCollected = false;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Overlap begin function
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	// Implement ICollectibleInterface
	virtual void Collect_Implementation();
	virtual bool CanBeCollected_Implementation() const;

	// Event dispatcher for when pickup is collected
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPickupCollected, int32, ResourceValue);
	UPROPERTY(BlueprintAssignable, Category = "Pickup")
	FOnPickupCollected OnPickupCollected;

	// Timeline update function
	UFUNCTION()
	void UpdateBob(float Value);

	// Start bobbing animation
	UFUNCTION(BlueprintCallable, Category = "Pickup")
	void StartBobbing();

	// Stop bobbing animation
	UFUNCTION(BlueprintCallable, Category = "Pickup")
	void StopBobbing();
};
