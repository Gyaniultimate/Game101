#include "Grenade.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "PhysicsEngine/RadialForceComponent.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "TacticalDoor.h"

AGrenade::AGrenade()
{
    PrimaryActorTick.bCanEverTick = false;

    // 1. The physical object
    GrenadeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GrenadeMesh"));
    RootComponent = GrenadeMesh;
    GrenadeMesh->SetSimulatePhysics(true);

    // NEW: Kill the infinite rolling and sliding
    GrenadeMesh->SetLinearDamping(1.5f);  // Slows down sliding across the floor
    GrenadeMesh->SetAngularDamping(5.0f); // Aggressively stops it from rolling

    // 2. The throwing physics (arc and bounce)
    ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileComp"));
    ProjectileMovement->InitialSpeed = 1000.f;
    ProjectileMovement->MaxSpeed = 1000.f;
    ProjectileMovement->bShouldBounce = true;
    ProjectileMovement->Bounciness = 0.1f;
    ProjectileMovement->ProjectileGravityScale = 2.0f;

    // NEW: Add high friction to the projectile component itself
    ProjectileMovement->Friction = 0.8f;

    // 3. The Explosion Force
    ExplosionForce = CreateDefaultSubobject<URadialForceComponent>(TEXT("ExplosionForce"));
    ExplosionForce->SetupAttachment(RootComponent);
    ExplosionForce->Radius = 600.f;
    ExplosionForce->ImpulseStrength = 200000.f; // Needs to be huge to throw heavy mannequins
    ExplosionForce->bImpulseVelChange = false;
    ExplosionForce->bAutoActivate = false; // Don't explode immediately!
}

void AGrenade::BeginPlay()
{
    Super::BeginPlay();

    // Start the 3 second fuse
    GetWorldTimerManager().SetTimer(ExplosionTimerHandle, this, &AGrenade::Explode, 3.0f, false);
}

void AGrenade::Explode()
{
    TArray<AActor*> FoundDoors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATacticalDoor::StaticClass(), FoundDoors);

    // 1. Raise the blast origin up 50 units so it doesn't get trapped inside the floor mesh
    FVector BlastOrigin = GetActorLocation() + FVector(0.0f, 0.0f, 50.0f);

    for (AActor* Actor : FoundDoors)
    {
        float Distance = FVector::Dist(Actor->GetActorLocation(), BlastOrigin);

        if (Distance <= ExplosionForce->Radius + 200.0f)
        {
            ATacticalDoor* Door = Cast<ATacticalDoor>(Actor);
            if (Door && Door->DoorMesh)
            {
                // Turn on physics
                Door->DoorMesh->SetSimulatePhysics(true);

                // 2. Steal the math from your Door Kick! Calculate the direction away from the grenade.
                FVector BlastDirection = Door->GetActorLocation() - BlastOrigin;
                BlastDirection.Normalize(); // Ensure the vector is just a direction

                // 3. Apply the exact same 5000 force multiplier that worked for your foot
                float BlastForceMultiplier = 5000.0f;
                Door->DoorMesh->AddImpulse(BlastDirection * BlastForceMultiplier, NAME_None, true);
            }
        }
    }

    // 4. Fire the standard radial force to push mannequins and other loose physics objects
    ExplosionForce->FireImpulse();

    // 5. Draw the debug sphere using the raised BlastOrigin
    DrawDebugSphere(GetWorld(), BlastOrigin, ExplosionForce->Radius, 32, FColor::Orange, false, 2.0f);

    Destroy();
}