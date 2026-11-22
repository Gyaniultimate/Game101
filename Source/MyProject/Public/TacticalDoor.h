#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TacticalDoor.generated.h"

UCLASS()
class MYPROJECT_API ATacticalDoor : public AActor
{
    GENERATED_BODY()

public:
    ATacticalDoor();

    // The physical door mesh
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UStaticMeshComponent* DoorMesh;

    // The function your player will call to kick the door
    UFUNCTION(BlueprintCallable, Category = "Interaction")
    void KickDoor(FVector KickDirection);
};