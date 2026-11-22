#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RoomManager.generated.h"

// Define our 4 room scenarios
UENUM(BlueprintType)
enum class ERoomState : uint8
{
    RS_HostageOnly UMETA(DisplayName = "Hostage Only"),
    RS_HostageAndMilitant UMETA(DisplayName = "Hostage and Militant"),
    RS_MilitantOnly UMETA(DisplayName = "Militant Only"),
    RS_ColluderTrap UMETA(DisplayName = "Colluder Trap")
};

UCLASS()
class MYPROJECT_API ARoomManager : public AActor
{
    GENERATED_BODY()

public:
    ARoomManager();

protected:
    virtual void BeginPlay() override;

public:
    // Shows the current state in the editor for debugging
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Room State")
    ERoomState CurrentRoomState;

    // These allow you to assign your Blueprinted NPCs in the Editor
    UPROPERTY(EditAnywhere, Category = "Spawning")
    TSubclassOf<AActor> HostageClass;

    UPROPERTY(EditAnywhere, Category = "Spawning")
    TSubclassOf<AActor> MilitantClass;

    UPROPERTY(EditAnywhere, Category = "Spawning")
    TSubclassOf<AActor> ColluderClass;

    // Visual anchor points for where NPCs will spawn
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class USceneComponent* RootComp;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class USceneComponent* CenterSpawnPoint;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class USceneComponent* UnderBedSpawnPoint;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class USceneComponent* CornerSpawnPoint;

    // The main logic function
    UFUNCTION(BlueprintCallable, Category = "Room Logic")
    void GenerateRoom();
};