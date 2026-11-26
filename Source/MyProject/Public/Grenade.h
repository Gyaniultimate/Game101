#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Grenade.generated.h"

class UStaticMeshComponent;
class UProjectileMovementComponent;
class URadialForceComponent;
class UPhysicalMaterial;
class USoundBase;
class UParticleSystem;
class UCameraShakeBase;
class UDamageType;

UCLASS()
class MYPROJECT_API AGrenade : public AActor
{
	GENERATED_BODY()

public:
	AGrenade();

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	// Main grenade body (Root, simulates realistic Chaos physics)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* GrenadeMesh;

	// Metallic fuze collar on top of the grenade body
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* FuseNeckMesh;

	// Safety lever / spoon that pops off when the grenade is armed/dropped
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* SpoonMesh;

	// Kept deactivated to preserve BP_Grenade compatibility without fighting Chaos physics
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UProjectileMovementComponent* ProjectileMovement;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	URadialForceComponent* ExplosionForce;

protected:
	// --- Realistic Drop & Physics Settings ---

	/** Mass of the grenade in kg (M67 frag grenade ~0.4 kg) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grenade|Physics", meta = (ClampMin = "0.05", ClampMax = "5.0"))
	float GrenadeMassKg = 0.40f;

	/** Restitution (bounciness) override: low value prevents rubber-ball bouncing */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grenade|Physics", meta = (ClampMin = "0.0", ClampMax = "0.5"))
	float BounceRestitution = 0.08f;

	/** Surface friction override so the grenade grips concrete/tile and settles fast */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grenade|Physics", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float SurfaceFriction = 0.85f;

	/** Linear damping to bleed off sliding momentum */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grenade|Physics", meta = (ClampMin = "0.0", ClampMax = "10.0"))
	float LinearDamping = 0.85f;

	/** Angular damping to stop endless rolling */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grenade|Physics", meta = (ClampMin = "0.0", ClampMax = "15.0"))
	float AngularDamping = 4.5f;

	/** Gentle forward speed when dropped (cm/s) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grenade|Drop", meta = (ClampMin = "0.0", ClampMax = "1500.0"))
	float DropForwardSpeed = 220.0f;

	/** Slight downward push when dropped so it feels heavy immediately (cm/s) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grenade|Drop", meta = (ClampMin = "0.0", ClampMax = "500.0"))
	float DropDownwardSpeed = 80.0f;

	/** Maximum upward rebound speed allowed after hitting the floor (cm/s) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grenade|Physics", meta = (ClampMin = "0.0", ClampMax = "300.0"))
	float MaxReboundZVelocity = 65.0f;

	/** Whether the safety spoon pops off with physics on BeginPlay */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grenade|Visuals")
	bool bPopSpoonOnDrop = true;

	// --- Fuse & Detonation ---

	/** Fuse duration in seconds before detonation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grenade|Explosion", meta = (ClampMin = "0.5", ClampMax = "10.0"))
	float FuseTime = 3.0f;

	/** Max radial damage at the center of the blast */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grenade|Explosion", meta = (ClampMin = "0.0", ClampMax = "500.0"))
	float MaxExplosionDamage = 150.0f;

	/** Minimum radial damage at the outer edge of the blast */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grenade|Explosion", meta = (ClampMin = "0.0", ClampMax = "200.0"))
	float MinExplosionDamage = 20.0f;

	/** Inner radius receiving full explosion damage */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grenade|Explosion", meta = (ClampMin = "10.0", ClampMax = "1000.0"))
	float DamageInnerRadius = 200.0f;

	/** Damage type class applied by radial blast */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grenade|Explosion")
	TSubclassOf<UDamageType> ExplosionDamageType;

	// --- Audio, VFX & Camera Feel ---

	/** Sound played when the grenade hits a hard surface */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grenade|Audio")
	USoundBase* ImpactSound;

	/** Sound played when the safety spoon pops off */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grenade|Audio")
	USoundBase* SpoonPopSound;

	/** Sound played on detonation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grenade|Audio")
	USoundBase* ExplosionSound;

	/** Optional Cascade particle system spawned on detonation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grenade|VFX")
	UParticleSystem* ExplosionParticles;

	/** Camera shake played for nearby players on detonation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grenade|VFX")
	TSubclassOf<UCameraShakeBase> ExplosionCameraShake;

	/** Draw debug blast sphere on detonation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grenade|Debug")
	bool bShowDebugBlastSphere = false;

	/** Runtime low-restitution physical material */
	UPROPERTY(Transient)
	UPhysicalMaterial* RuntimePhysMaterial;

	FTimerHandle ExplosionTimerHandle;
	float LastImpactSoundTime = -1.0f;

	/** Applies physical material, mass, and damping settings */
	void ApplyRealisticPhysicsSettings();

	/** Detaches and pops off the safety spoon */
	void PopSafetySpoon();

	/** Handles floor/wall impacts to clamp bounce, play clink audio, and emit AI noise */
	UFUNCTION()
	void OnGrenadeHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	/** Blueprint hook for custom Niagara/Cascade explosion effects */
	UFUNCTION(BlueprintImplementableEvent, Category = "Grenade|VFX", meta = (DisplayName = "On Grenade Explode"))
	void BP_OnExplode(FVector BlastOrigin);

public:
	/** Triggers detonation */
	UFUNCTION(BlueprintCallable, Category = "Grenade")
	void Explode();
};
