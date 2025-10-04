// Fill out your copyright notice in the Description page of Project Settings.

#include "BasePickup.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Character.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "TimerManager.h"

// Sets default values
ABasePickup::ABasePickup()
{
 	// Set this actor to call Tick() every frame
	PrimaryActorTick.bCanEverTick = true;

	// Create root component
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

	// Create collision sphere
	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->SetupAttachment(RootComponent);
	CollisionSphere->SetSphereRadius(100.0f);
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	// Create static mesh
	PickupMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PickupMesh"));
	PickupMesh->SetupAttachment(RootComponent);
	PickupMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Set default mesh (basic cube)
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube"));
	if (CubeMesh.Succeeded())
	{
		PickupMesh->SetStaticMesh(CubeMesh.Object);
	}

	// Set default material
	static ConstructorHelpers::FObjectFinder<UMaterial> DefaultMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial"));
	if (DefaultMaterial.Succeeded())
	{
		PickupMesh->SetMaterial(0, DefaultMaterial.Object);
	}

	// Bind overlap event
	CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &ABasePickup::OnOverlapBegin);
}

// Called when the game starts or when spawned
void ABasePickup::BeginPlay()
{
	Super::BeginPlay();
	
	// Store initial Z position for bobbing
	InitialZ = GetActorLocation().Z;

	// Start bobbing animation
	StartBobbing();
}

// Called every frame
void ABasePickup::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Rotate the pickup mesh
	if (PickupMesh && !bIsCollected)
	{
		FRotator CurrentRotation = PickupMesh->GetRelativeRotation();
		CurrentRotation.Yaw += RotationSpeed * DeltaTime;
		PickupMesh->SetRelativeRotation(CurrentRotation);
	}

	// Update timeline
	if (BobTimeline.IsPlaying())
	{
		BobTimeline.TickTimeline(DeltaTime);
	}
}

// Overlap begin function
void ABasePickup::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// Check if the overlapping actor is a character
	if (ACharacter* Character = Cast<ACharacter>(OtherActor))
	{
		if (CanBeCollected())
		{
			Collect();
		}
	}
}

// Implement ICollectibleInterface
void ABasePickup::Collect_Implementation()
{
	if (bIsCollected) return;

	bIsCollected = true;

	// Broadcast the collection event
	OnPickupCollected.Broadcast(ResourceValue);

	// Print debug message
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, 
			FString::Printf(TEXT("Collected pickup! Resource value: %d"), ResourceValue));
	}

	// Destroy the actor after a short delay
	FTimerHandle DestroyTimer;
	GetWorldTimerManager().SetTimer(DestroyTimer, [this]()
	{
		Destroy();
	}, 0.1f, false);
}

bool ABasePickup::CanBeCollected_Implementation() const
{
	return !bIsCollected;
}

// Timeline update function
void ABasePickup::UpdateBob(float Value)
{
	if (PickupMesh)
	{
		FVector CurrentLocation = PickupMesh->GetRelativeLocation();
		CurrentLocation.Z = InitialZ + (Value * BobAmplitude);
		PickupMesh->SetRelativeLocation(CurrentLocation);
	}
}

// Start bobbing animation
void ABasePickup::StartBobbing()
{
	if (BobCurve)
	{
		// Create timeline if it doesn't exist
		if (BobTimeline.GetTimelineLength() == 0.0f)
		{
			FOnTimelineFloat TimelineCallback;
			TimelineCallback.BindUFunction(this, FName("UpdateBob"));
			BobTimeline.AddInterpFloat(BobCurve, TimelineCallback);
			BobTimeline.SetLooping(true);
		}

		BobTimeline.PlayFromStart();
	}
}

// Stop bobbing animation
void ABasePickup::StopBobbing()
{
	if (BobTimeline.IsPlaying())
	{
		BobTimeline.Stop();
	}
}
