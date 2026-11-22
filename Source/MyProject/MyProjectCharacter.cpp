// Copyright Epic Games, Inc. All Rights Reserved.

#include "MyProjectCharacter.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "MyProject.h"
#include "TacticalDoor.h"
#include "DrawDebugHelpers.h" 
#include "Grenade.h"// lets us see the invisible line for testing

AMyProjectCharacter::AMyProjectCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);
	
	// Create the first person mesh that will be viewed only by this character's owner
	FirstPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("First Person Mesh"));

	FirstPersonMesh->SetupAttachment(GetMesh());
	FirstPersonMesh->SetOnlyOwnerSee(true);
	FirstPersonMesh->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::FirstPerson;
	FirstPersonMesh->SetCollisionProfileName(FName("NoCollision"));

	// Create the Camera Component	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("First Person Camera"));
	FirstPersonCameraComponent->SetupAttachment(FirstPersonMesh, FName("head"));
	FirstPersonCameraComponent->SetRelativeLocationAndRotation(FVector(-2.8f, 5.89f, 0.0f), FRotator(0.0f, 90.0f, -90.0f));
	FirstPersonCameraComponent->bUsePawnControlRotation = true;
	FirstPersonCameraComponent->bEnableFirstPersonFieldOfView = true;
	FirstPersonCameraComponent->bEnableFirstPersonScale = true;
	FirstPersonCameraComponent->FirstPersonFieldOfView = 70.0f;
	FirstPersonCameraComponent->FirstPersonScale = 0.6f;

	// configure the character comps
	GetMesh()->SetOwnerNoSee(true);
	GetMesh()->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::WorldSpaceRepresentation;

	GetCapsuleComponent()->SetCapsuleSize(34.0f, 96.0f);

	// Configure character movement
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;
	GetCharacterMovement()->AirControl = 0.5f;
}

void AMyProjectCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{	
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AMyProjectCharacter::DoJumpStart);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &AMyProjectCharacter::DoJumpEnd);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMyProjectCharacter::MoveInput);

		// Looking/Aiming
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AMyProjectCharacter::LookInput);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AMyProjectCharacter::LookInput);

		//kicking

		PlayerInputComponent->BindAction("Kick", IE_Pressed, this, &AMyProjectCharacter::KickAction);

		//throwing grenade
		PlayerInputComponent->BindAction("Grenade", IE_Pressed, this, &AMyProjectCharacter::ThrowGrenade);
	}
	else
	{
		UE_LOG(LogMyProject, Error, TEXT("'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}


void AMyProjectCharacter::MoveInput(const FInputActionValue& Value)
{
	// get the Vector2D move axis
	FVector2D MovementVector = Value.Get<FVector2D>();

	// pass the axis values to the move input
	DoMove(MovementVector.X, MovementVector.Y);

}

void AMyProjectCharacter::LookInput(const FInputActionValue& Value)
{
	// get the Vector2D look axis
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// pass the axis values to the aim input
	DoAim(LookAxisVector.X, LookAxisVector.Y);

}

void AMyProjectCharacter::DoAim(float Yaw, float Pitch)
{
	if (GetController())
	{
		// pass the rotation inputs
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void AMyProjectCharacter::DoMove(float Right, float Forward)
{
	if (GetController())
	{
		// pass the move inputs
		AddMovementInput(GetActorRightVector(), Right);
		AddMovementInput(GetActorForwardVector(), Forward);
	}
}

void AMyProjectCharacter::DoJumpStart()
{
	// pass Jump to the character
	Jump();
}

void AMyProjectCharacter::DoJumpEnd()
{
	// pass StopJumping to the character
	StopJumping();
}
void AMyProjectCharacter::KickAction()
{
	// 1. Get the camera's location and where it is pointing
	FVector StartLocation = GetFirstPersonCameraComponent()->GetComponentLocation();
	FVector ForwardVector = GetFirstPersonCameraComponent()->GetForwardVector();

	// 2. Set the reach of the kick (e.g., 200 units/cm in front of the player)
	float KickReach = 200.0f;
	FVector EndLocation = StartLocation + (ForwardVector * KickReach);

	// 3. Prepare the Line Trace
	FHitResult HitResult;
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(this); // Don't kick yourself!

	// 4. Fire the invisible raycast
	bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, StartLocation, EndLocation, ECC_Visibility, CollisionParams);

	// 5. Draw a red line for 2 seconds so we can see the kick working (remove this later)
	DrawDebugLine(GetWorld(), StartLocation, EndLocation, FColor::Red, false, 2.0f, 0, 2.0f);

	// 6. If we hit something, check if it was a Tactical Door
	if (bHit)
	{
		AActor* HitActor = HitResult.GetActor();
		if (HitActor)
		{
			// Try to cast the hit actor to a Tactical Door
			ATacticalDoor* HitDoor = Cast<ATacticalDoor>(HitActor);
			if (HitDoor)
			{
				// We hit a door! Call the kick function and pass the direction we are looking
				HitDoor->KickDoor(ForwardVector);
			}
		}
	}
}

void AMyProjectCharacter::ThrowGrenade()
{
	if (GrenadeClass)
	{
		// Spawn the grenade slightly in front of the camera so it doesn't hit our own face
		FVector SpawnLocation = GetFirstPersonCameraComponent()->GetComponentLocation() + (GetFirstPersonCameraComponent()->GetForwardVector() * 50.0f);
		FRotator SpawnRotation = GetFirstPersonCameraComponent()->GetComponentRotation();

		GetWorld()->SpawnActor<AGrenade>(GrenadeClass, SpawnLocation, SpawnRotation);
	}
}
