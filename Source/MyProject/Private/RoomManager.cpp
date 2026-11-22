#include "RoomManager.h"
#include "Engine/World.h"
#include "Math/UnrealMathUtility.h"

ARoomManager::ARoomManager()
{
    // Turn off ticking to save performance, we only run this once at start
    PrimaryActorTick.bCanEverTick = false;

    // Create the physical anchor points
    RootComp = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
    RootComponent = RootComp;

    CenterSpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("CenterSpawnPoint"));
    CenterSpawnPoint->SetupAttachment(RootComponent);

    UnderBedSpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("UnderBedSpawnPoint"));
    UnderBedSpawnPoint->SetupAttachment(RootComponent);

    CornerSpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("CornerSpawnPoint"));
    CornerSpawnPoint->SetupAttachment(RootComponent);
}

void ARoomManager::BeginPlay()
{
    Super::BeginPlay();
    GenerateRoom();
}

void ARoomManager::GenerateRoom()
{
    // 1. Pick a random state (0, 1, 2, or 3)
    int32 RandomState = FMath::RandRange(0, 3);
    CurrentRoomState = static_cast<ERoomState>(RandomState);

    // 2. Set up spawn rules (always spawn, even if overlapping slightly)
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    // 3. Execute the spawn based on the random state
    switch (CurrentRoomState)
    {
    case ERoomState::RS_HostageOnly:
        if (HostageClass)
        {
            GetWorld()->SpawnActor<AActor>(HostageClass, CornerSpawnPoint->GetComponentLocation(), CornerSpawnPoint->GetComponentRotation(), SpawnParams);
        }
        break;

    case ERoomState::RS_HostageAndMilitant:
        if (HostageClass && MilitantClass)
        {
            GetWorld()->SpawnActor<AActor>(HostageClass, CornerSpawnPoint->GetComponentLocation(), CornerSpawnPoint->GetComponentRotation(), SpawnParams);
            GetWorld()->SpawnActor<AActor>(MilitantClass, UnderBedSpawnPoint->GetComponentLocation(), UnderBedSpawnPoint->GetComponentRotation(), SpawnParams);
        }
        break;

    case ERoomState::RS_MilitantOnly:
        if (MilitantClass)
        {
            GetWorld()->SpawnActor<AActor>(MilitantClass, CenterSpawnPoint->GetComponentLocation(), CenterSpawnPoint->GetComponentRotation(), SpawnParams);
        }
        break;

    case ERoomState::RS_ColluderTrap:
        if (ColluderClass && MilitantClass)
        {
            // Colluder stands in the center to distract you
            GetWorld()->SpawnActor<AActor>(ColluderClass, CenterSpawnPoint->GetComponentLocation(), CenterSpawnPoint->GetComponentRotation(), SpawnParams);
            // Militant hides under the bed
            GetWorld()->SpawnActor<AActor>(MilitantClass, UnderBedSpawnPoint->GetComponentLocation(), UnderBedSpawnPoint->GetComponentRotation(), SpawnParams);
        }
        break;
    }
}