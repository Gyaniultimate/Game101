#include "Grenade.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "PhysicsEngine/RadialForceComponent.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/DamageType.h"
#include "Camera/CameraShakeBase.h"
#include "TacticalDoor.h"

AGrenade::AGrenade()
{
	PrimaryActorTick.bCanEverTick = false;

	// 1. Main physical grenade body (Chaos rigid-body physics)
	GrenadeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GrenadeMesh"));
	RootComponent = GrenadeMesh;
	GrenadeMesh->SetCollisionProfileName(TEXT("PhysicsActor"));
	GrenadeMesh->SetSimulatePhysics(true);
	GrenadeMesh->SetNotifyRigidBodyCollision(true);
	GrenadeMesh->SetUseCCD(true); // Prevent tunneling through floors/doors
	GrenadeMesh->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;

	// 2. Fuze neck collar on top of the grenade body
	FuseNeckMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FuseNeckMesh"));
	FuseNeckMesh->SetupAttachment(GrenadeMesh);
	FuseNeckMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FuseNeckMesh->SetRelativeLocation(FVector::ZeroVector);

	// 3. Safety lever / spoon on the side (aligned to grenade origin)
	SpoonMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SpoonMesh"));
	SpoonMesh->SetupAttachment(GrenadeMesh);
	SpoonMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SpoonMesh->SetRelativeLocation(FVector::ZeroVector);

	// 4. Keep ProjectileMovement deactivated so it never fights Chaos physics
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileComp"));
	ProjectileMovement->bAutoActivate = false;
	ProjectileMovement->SetUpdatedComponent(nullptr);

	// 5. Radial explosion force
	ExplosionForce = CreateDefaultSubobject<URadialForceComponent>(TEXT("ExplosionForce"));
	ExplosionForce->SetupAttachment(RootComponent);
	ExplosionForce->Radius = 600.0f;
	ExplosionForce->ImpulseStrength = 200000.0f;
	ExplosionForce->bImpulseVelChange = false;
	ExplosionForce->bAutoActivate = false;

	ExplosionDamageType = UDamageType::StaticClass();
}

void AGrenade::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyRealisticPhysicsSettings();
}

void AGrenade::BeginPlay()
{
	Super::BeginPlay();

	// Ensure ProjectileMovement stays completely off
	if (ProjectileMovement)
	{
		ProjectileMovement->Deactivate();
		ProjectileMovement->SetUpdatedComponent(nullptr);
	}

	ApplyRealisticPhysicsSettings();

	if (GrenadeMesh)
	{
		GrenadeMesh->OnComponentHit.AddDynamic(this, &AGrenade::OnGrenadeHit);

		// Give the grenade a realistic weighty drop velocity + subtle tumble
		const FVector DropVelocity = (GetActorForwardVector() * DropForwardSpeed) + FVector(0.0f, 0.0f, -DropDownwardSpeed);
		GrenadeMesh->SetPhysicsLinearVelocity(DropVelocity, false);

		const FVector TumbleTorque(
			FMath::RandRange(-140.0f, 140.0f),
			FMath::RandRange(180.0f, 320.0f),
			FMath::RandRange(-90.0f, 90.0f)
		);
		GrenadeMesh->AddAngularImpulseInDegrees(TumbleTorque, NAME_None, true);
	}

	// Pop the safety spoon when armed/dropped
	if (bPopSpoonOnDrop)
	{
		PopSafetySpoon();
	}

	// Start the fuse timer
	GetWorldTimerManager().SetTimer(ExplosionTimerHandle, this, &AGrenade::Explode, FuseTime, false);
}

void AGrenade::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	GetWorldTimerManager().ClearTimer(ExplosionTimerHandle);
}

void AGrenade::ApplyRealisticPhysicsSettings()
{
	if (!GrenadeMesh)
	{
		return;
	}

	// Create a low-restitution, high-friction physical material so it thuds instead of bouncing like a ball
	if (!RuntimePhysMaterial)
	{
		RuntimePhysMaterial = NewObject<UPhysicalMaterial>(this, TEXT("GrenadeRuntimePhysMat"));
	}

	if (RuntimePhysMaterial)
	{
		RuntimePhysMaterial->Restitution = BounceRestitution;
		RuntimePhysMaterial->bOverrideRestitutionCombineMode = true;
		RuntimePhysMaterial->RestitutionCombineMode = EFrictionCombineMode::Min;

		RuntimePhysMaterial->Friction = SurfaceFriction;
		RuntimePhysMaterial->StaticFriction = FMath::Clamp(SurfaceFriction + 0.05f, 0.0f, 1.0f);
		RuntimePhysMaterial->bOverrideFrictionCombineMode = true;
		RuntimePhysMaterial->FrictionCombineMode = EFrictionCombineMode::Max;

		RuntimePhysMaterial->Density = 2.5f;

		GrenadeMesh->SetPhysMaterialOverride(RuntimePhysMaterial);
	}

	GrenadeMesh->SetMassOverrideInKg(NAME_None, GrenadeMassKg, true);
	GrenadeMesh->SetLinearDamping(LinearDamping);
	GrenadeMesh->SetAngularDamping(AngularDamping);
}

void AGrenade::PopSafetySpoon()
{
	if (!SpoonMesh || !SpoonMesh->GetStaticMesh())
	{
		return;
	}

	SpoonMesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	SpoonMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	SpoonMesh->SetCollisionResponseToAllChannels(ECR_Block);
	SpoonMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	SpoonMesh->SetSimulatePhysics(true);
	SpoonMesh->SetLinearDamping(1.0f);
	SpoonMesh->SetAngularDamping(2.0f);

	const FVector PopDir = (GetActorRightVector() * 140.0f) + FVector(0.0f, 0.0f, 160.0f);
	SpoonMesh->AddImpulse(PopDir, NAME_None, true);
	SpoonMesh->AddAngularImpulseInDegrees(FVector(360.0f, -240.0f, 180.0f), NAME_None, true);

	if (SpoonPopSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, SpoonPopSound, GetActorLocation(), 0.7f, FMath::RandRange(0.95f, 1.05f));
	}
}

void AGrenade::OnGrenadeHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (!GrenadeMesh)
	{
		return;
	}

	// Clamp upward bounce velocity so it settles with heavy metallic thuds
	FVector CurrentVel = GrenadeMesh->GetPhysicsLinearVelocity();
	if (CurrentVel.Z > MaxReboundZVelocity)
	{
		CurrentVel.Z = MaxReboundZVelocity;
		CurrentVel.X *= 0.65f;
		CurrentVel.Y *= 0.65f;
		GrenadeMesh->SetPhysicsLinearVelocity(CurrentVel, false);
	}

	// Play metallic impact clink/thud with cooldown so rolling doesn't spam audio
	const float CurrentTime = GetWorld()->GetTimeSeconds();
	const float ImpactSpeed = NormalImpulse.Size();

	if (ImpactSpeed > 15.0f && (CurrentTime - LastImpactSoundTime) > 0.15f)
	{
		LastImpactSoundTime = CurrentTime;

		if (ImpactSound)
		{
			const float Volume = FMath::Clamp(ImpactSpeed / 300.0f, 0.15f, 1.0f);
			UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, Hit.ImpactPoint, Volume, FMath::RandRange(0.92f, 1.08f));
		}

		// Alert nearby AI to the grenade hitting the floor
		MakeNoise(0.6f, GetInstigator(), Hit.ImpactPoint, 700.0f, FName("GrenadeImpact"));
	}
}

void AGrenade::Explode()
{
	// Raise the blast origin slightly so it doesn't clip into the floor mesh
	const FVector BlastOrigin = GetActorLocation() + FVector(0.0f, 0.0f, 50.0f);
	const float BlastRadius = ExplosionForce ? ExplosionForce->Radius : 600.0f;

	// 1. Breach nearby tactical doors
	TArray<AActor*> FoundDoors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATacticalDoor::StaticClass(), FoundDoors);

	for (AActor* Actor : FoundDoors)
	{
		const float Distance = FVector::Dist(Actor->GetActorLocation(), BlastOrigin);
		if (Distance <= BlastRadius + 200.0f)
		{
			ATacticalDoor* Door = Cast<ATacticalDoor>(Actor);
			if (Door && Door->DoorMesh)
			{
				Door->DoorMesh->SetSimulatePhysics(true);

				FVector BlastDirection = Door->GetActorLocation() - BlastOrigin;
				BlastDirection.Normalize();

				const float BlastForceMultiplier = 5000.0f;
				Door->DoorMesh->AddImpulse(BlastDirection * BlastForceMultiplier, NAME_None, true);
			}
		}
	}

	// 2. Apply radial damage with realistic distance falloff
	TArray<AActor*> IgnoredActors;
	IgnoredActors.Add(this);

	AController* InstigatorController = GetInstigator() ? GetInstigator()->GetController() : nullptr;
	UGameplayStatics::ApplyRadialDamageWithFalloff(
		GetWorld(),
		MaxExplosionDamage,
		MinExplosionDamage,
		BlastOrigin,
		DamageInnerRadius,
		BlastRadius,
		1.5f,
		ExplosionDamageType ? ExplosionDamageType : TSubclassOf<UDamageType>(UDamageType::StaticClass()),
		IgnoredActors,
		this,
		InstigatorController
	);

	// 3. Fire radial physics impulse
	if (ExplosionForce)
	{
		ExplosionForce->FireImpulse();
	}

	// 4. Audio, VFX, Camera Shake & AI Perception Noise
	if (ExplosionSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ExplosionSound, BlastOrigin);
	}

	if (ExplosionParticles)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionParticles, BlastOrigin, FRotator::ZeroRotator, true);
	}

	if (ExplosionCameraShake)
	{
		UGameplayStatics::PlayWorldCameraShake(GetWorld(), ExplosionCameraShake, BlastOrigin, 0.0f, BlastRadius * 2.5f, 1.0f);
	}

	MakeNoise(3.0f, GetInstigator(), BlastOrigin, 2000.0f, FName("Explosion"));

	// Blueprint VFX hook (e.g. Niagara or BP_ShooterGrenadeExplosion)
	BP_OnExplode(BlastOrigin);

	if (bShowDebugBlastSphere)
	{
		DrawDebugSphere(GetWorld(), BlastOrigin, BlastRadius, 32, FColor::Orange, false, 2.0f);
	}

	Destroy();
}
