// Copyright Epic Games, Inc. All Rights Reserved.

#include "RococoTestFPSPickUpComponent.h"

URococoTestFPSPickUpComponent::URococoTestFPSPickUpComponent()
{
	// Setup the Sphere Collision
	SphereRadius = 32.f;
}

void URococoTestFPSPickUpComponent::BeginPlay()
{
	Super::BeginPlay();

	// Register our Overlap Event
	OnComponentBeginOverlap.AddDynamic(this, &URococoTestFPSPickUpComponent::OnSphereBeginOverlap);
}

void URococoTestFPSPickUpComponent::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// Checking if it is a First Person Character overlapping
	ARococoTestFPSCharacter* Character = Cast<ARococoTestFPSCharacter>(OtherActor);
	if(Character != nullptr)
	{
		// Notify that the actor is being picked up
		OnPickUp.Broadcast(Character);

		// Unregister from the Overlap Event so it is no longer triggered
		OnComponentBeginOverlap.RemoveAll(this);
	}
}
