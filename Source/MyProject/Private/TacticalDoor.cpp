#include "TacticalDoor.h"
#include "Components/StaticMeshComponent.h"

ATacticalDoor::ATacticalDoor()
{
    PrimaryActorTick.bCanEverTick = false;

    // Create the mesh component and set it as the root
    DoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorMesh"));
    RootComponent = DoorMesh;

    // We want the door to be solid to block the player and enemies initially
    DoorMesh->SetCollisionProfileName(TEXT("BlockAll"));

    // Make sure physics is off until the door is kicked
    DoorMesh->SetSimulatePhysics(false);
}

void ATacticalDoor::KickDoor(FVector KickDirection)
{
    // 1. Turn on physics so the door can be pushed
    DoorMesh->SetSimulatePhysics(true);

    // 2. Multiply the player's facing direction by a massive force
    float KickForceMultiplier = 5000.0f;
    FVector AppliedForce = KickDirection * KickForceMultiplier;

    // 3. Apply the force to blast the door open
    DoorMesh->AddImpulse(AppliedForce, NAME_None, true);
}