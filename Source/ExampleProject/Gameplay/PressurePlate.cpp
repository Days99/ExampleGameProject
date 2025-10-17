// Fill out your copyright notice in the Description page of Project Settings.

#include "PressurePlate.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Character.h"
#include "../Core/ExampleProjectCharacter.h"

// Sets default values
APressurePlate::APressurePlate()
{
	PrimaryActorTick.bCanEverTick = true;

	// Enable replication for this actor
	bReplicates = true;
	bAlwaysRelevant = true; 

	// Create root component
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

	// Create plate mesh
	PlateMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlateMesh"));
	PlateMesh->SetupAttachment(RootComponent);
	PlateMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Create trigger box
	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	TriggerBox->SetupAttachment(RootComponent);
	TriggerBox->SetBoxExtent(FVector(100.0f, 100.0f, 50.0f));
	TriggerBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	// Bind overlap events
	TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &APressurePlate::OnOverlapBegin);
	TriggerBox->OnComponentEndOverlap.AddDynamic(this, &APressurePlate::OnOverlapEnd);

	// Initialize state
	bIsActivated = false;
	PlayersOnPlate = 0;
	DynamicMaterialInstance = nullptr;

	// Set default colors
	InactiveColor = FLinearColor(0.5f, 0.5f, 0.5f, 1.0f); // Gray
	ActiveColor = FLinearColor(0.0f, 1.0f, 0.0f, 1.0f);   // Green
}

// Called when the game starts or when spawned
void APressurePlate::BeginPlay()
{
	Super::BeginPlay();
	
	// Create dynamic material instance once at startup
	if (PlateMesh && PlateMesh->GetNumMaterials() > 0)
	{
		DynamicMaterialInstance = PlateMesh->CreateDynamicMaterialInstance(0);
		
		if (DynamicMaterialInstance)
		{
			UE_LOG(LogTemp, Log, TEXT("PressurePlate: Dynamic material instance created successfully"));
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("PressurePlate: Failed to create dynamic material instance!"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("PressurePlate: No mesh or materials found!"));
	}
	
	// Initialize visual state
	UpdatePlateVisuals();
}

// This is the RepNotify function - it's called automatically on clients when bIsActivated changes
void APressurePlate::OnRep_IsActivated()
{
	// Log for debugging - shows this is called on clients
	FString RoleString = HasAuthority() ? TEXT("Server") : TEXT("Client");
	UE_LOG(LogTemp, Log, TEXT("PressurePlate::OnRep_IsActivated called on %s - New state: %s"), 
		*RoleString, bIsActivated ? TEXT("ACTIVATED") : TEXT("DEACTIVATED"));

	// Update visuals when state changes
	UpdatePlateVisuals();

	// Here you could add sound effects, particle effects, etc.
	// All visual feedback should go here so it's synchronized across all clients
}

void APressurePlate::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, 
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// Only run on server
	if (!HasAuthority())
		return;

	// Check if it's a player character
	if (AExampleProjectCharacter* Character = Cast<AExampleProjectCharacter>(OtherActor))
	{
		PlayersOnPlate++;
		
		UE_LOG(LogTemp, Log, TEXT("Player stepped on pressure plate. Players on plate: %d"), PlayersOnPlate);

		// Activate if we have at least one player
		if (PlayersOnPlate > 0 && !bIsActivated)
		{
			bIsActivated = true;
			UE_LOG(LogTemp, Warning, TEXT("Pressure plate ACTIVATED!"));
			
			// The RepNotify will be called automatically on clients
			// We also need to update visuals on server
			UpdatePlateVisuals();
		}
	}
}

void APressurePlate::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, 
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	// Only run on server
	if (!HasAuthority())
		return;

	// Check if it's a player character
	if (AExampleProjectCharacter* Character = Cast<AExampleProjectCharacter>(OtherActor))
	{
		PlayersOnPlate--;
		if (PlayersOnPlate < 0) PlayersOnPlate = 0; // Safety check

		UE_LOG(LogTemp, Log, TEXT("Player left pressure plate. Players on plate: %d"), PlayersOnPlate);

		// Deactivate if no players remain
		if (PlayersOnPlate == 0 && bIsActivated)
		{
			bIsActivated = false;
			UE_LOG(LogTemp, Warning, TEXT("Pressure plate DEACTIVATED!"));
			
			// Update visuals on server
			UpdatePlateVisuals();
		}
	}
}

void APressurePlate::UpdatePlateVisuals()
{
	// Update the material parameters using the cached dynamic material instance
	if (DynamicMaterialInstance)
	{
		UE_LOG(LogTemp, Log, TEXT("PressurePlate: Updating material color to %s"), bIsActivated ? TEXT("Active") : TEXT("Inactive"));
		FLinearColor CurrentColor = bIsActivated ? ActiveColor : InactiveColor;
		DynamicMaterialInstance->SetVectorParameterValue(FName("BaseColor"), CurrentColor);
		DynamicMaterialInstance->SetVectorParameterValue(FName("Color"), CurrentColor);
	}

	// Optionally adjust plate height for visual feedback
	if (PlateMesh)
	{
		FVector CurrentLocation = PlateMesh->GetRelativeLocation();
		float TargetZ = bIsActivated ? -10.0f : 0.0f; // Press down when activated
		PlateMesh->SetRelativeLocation(FVector(CurrentLocation.X, CurrentLocation.Y, TargetZ));
	}
}

// Called every frame
void APressurePlate::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Register replicated properties
void APressurePlate::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	// Register bIsActivated for replication with RepNotify
	DOREPLIFETIME(APressurePlate, bIsActivated);
}

