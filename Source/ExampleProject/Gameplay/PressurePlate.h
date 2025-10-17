// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PressurePlate.generated.h"

UCLASS()
class EXAMPLEPROJECT_API APressurePlate : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APressurePlate();

	// Visual components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UStaticMeshComponent* PlateMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UBoxComponent* TriggerBox;

	// Replicated property with RepNotify - demonstrates advanced replication
	UPROPERTY(ReplicatedUsing=OnRep_IsActivated, VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle")
	bool bIsActivated;

	// Returns whether this plate is currently activated
	UFUNCTION(BlueprintPure, Category = "Puzzle")
	bool IsActivated() const { return bIsActivated; }

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// RepNotify function - called automatically when bIsActivated changes on clients
	UFUNCTION()
	void OnRep_IsActivated();

	// Overlap events
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, 
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, 
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	// Visual feedback functions
	void UpdatePlateVisuals();

	// Color materials for visual feedback
	UPROPERTY(EditAnywhere, Category = "Visual")
	FLinearColor InactiveColor;

	UPROPERTY(EditAnywhere, Category = "Visual")
	FLinearColor ActiveColor;

	// Dynamic material instance (created once at BeginPlay)
	UPROPERTY()
	class UMaterialInstanceDynamic* DynamicMaterialInstance;

	// Track number of players on plate (server only)
	int32 PlayersOnPlate;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Required for replication
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};

