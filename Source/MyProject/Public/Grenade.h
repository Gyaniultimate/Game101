#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Grenade.generated.h"

UCLASS()
class MYPROJECT_API AGrenade : public AActor
{
    GENERATED_BODY()

public:
    AGrenade();

protected:
    virtual void BeginPlay() override;

public:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UStaticMeshComponent* GrenadeMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UProjectileMovementComponent* ProjectileMovement;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class URadialForceComponent* ExplosionForce;

    // Function that runs when the fuse runs out
    void Explode();

    // Timer to act as our fuse
    FTimerHandle ExplosionTimerHandle;
};