// Copyright EmbraceIT Ltd

#include "TankAimingComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TankBarrel.h"
#include "TankTurret.h"
#include "Projectile.h"



// Sets default values for this component's properties
UTankAimingComponent::UTankAimingComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	// ...
}

void UTankAimingComponent::BeginPlay()
{
	Super::BeginPlay();

	// So that first fire is after initial reload
	LastFireTime = FPlatformTime::Seconds();


}

void UTankAimingComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	if (RoundsLeft <= 0)
	{
		FiringState = EFiringState::OutOfAmmo;
	}
	else if ((FPlatformTime::Seconds() - LastFireTime) < ReloadTimeInSeconds)
	{
		FiringState = EFiringState::Reloading;
	}
	else if(IsBarrelMoving())
	{
		FiringState = EFiringState::Aiming;
	}
	else
	{
		FiringState = EFiringState::Locked;
	}
	
}


void UTankAimingComponent::Initialize(UTankBarrel* BarrelToSet, UTankTurret* TurretToSet)
{
	Barrel = BarrelToSet;
	Turret = TurretToSet;
}

float UTankAimingComponent::GetMaxShootingRange()
{
	return MaxShootingRange;
}

EFiringState UTankAimingComponent::GetFiringState() const
{
	return FiringState;
}

int32 UTankAimingComponent::GetRoundsLeft() const
{
	return RoundsLeft;
}

bool UTankAimingComponent::GetPrecisionSightVisibility()
{
	return bIsPrecisionSightVisible;
}

void UTankAimingComponent::PrecisionAim(bool bShowPrecisionSight)
{	
		bIsPrecisionSightVisible = bShowPrecisionSight;
}


void UTankAimingComponent::Fire()
{
	if(FiringState == EFiringState::Aiming || FiringState == EFiringState::Locked)
	{
		if (!ensure(Barrel) && !ensure(ProjectileBlueprint)) { return; }
		// Spawn a projectile at the socket location on the barrel
		auto Projectile = GetWorld()->SpawnActor<AProjectile>(
			ProjectileBlueprint,
			Barrel->GetSocketLocation("Projectile"),
			Barrel->GetSocketRotation("Projectile")
			);

		Projectile->LaunchProjectile(LaunchSpeed);
		LastFireTime = FPlatformTime::Seconds();
		RoundsLeft--;
	}

}


void UTankAimingComponent::AimAt(FVector HitLocation)
{
	if (!ensure(Barrel)) { return; }
	FVector OutLaunchVelocity;
	FVector StartLocation = Barrel->GetSocketLocation(FName("Projectile"));
	bool bHasAimSolution = UGameplayStatics::SuggestProjectileVelocity(
		this,
		OutLaunchVelocity,
		StartLocation,
		HitLocation,
		LaunchSpeed,
		false,
		0,
		0,
		ESuggestProjVelocityTraceOption::DoNotTrace
	);
	if(bHasAimSolution)
	{
		auto OurTankName = GetOwner()->GetName();
		AimDirection = OutLaunchVelocity.GetSafeNormal();
		MoveBarrelTowards(AimDirection);
	}
	// if no solution is found do nothing
}

void UTankAimingComponent::MoveBarrelTowards(FVector AimDirection)
{
	if (!ensure(Barrel && Turret)) { return; } // or if(!ensure(Barrel) || !ensure(Turret)) to have separate messages
	auto AimAsRotator = AimDirection.Rotation();
	auto BarrelRotator = Barrel->GetForwardVector().Rotation();
	auto DeltaRotator = AimAsRotator - BarrelRotator;
	Barrel->Elevate(DeltaRotator.Pitch);

	// TODO: find reason for turret shudders. 
	// DeltaRotator.Yaw seems to be the culprit because it alternates from minus to plus for a single AimDirection, 
	// hence the turret tries to move in both directions at the same time and shudders

	// In the meantime, with a crossproduct, as used for the tank movement, it works just fine!
	auto RightMove = FVector::CrossProduct(Barrel->GetForwardVector(), AimDirection).Z;

	Turret->Rotate(RightMove * 10); // multiplied by 10, because otherwise was too slow, and is clamped from -1 to 1 in the TankTurret's Rotate() anyway

	//// The barrel position is used for the turret too, so no need to have a separate MoveTurretTowards() method
	//if (FMath::Abs(DeltaRotator.Yaw) < 180) // always yaw the shortest way
	//{ 
	//	Turret->Rotate(DeltaRotator.Yaw);
	//
	//}
	//else
	//{
	//	Turret->Rotate(-DeltaRotator.Yaw);
	//}

		//UE_LOG(LogTemp, Warning, TEXT("AimAsRotator is: %s"), *(AimAsRotator.ToString()))
		//UE_LOG(LogTemp, Warning, TEXT("BarrelRotator is: %s"), *(BarrelRotator.ToString()))
		//UE_LOG(LogTemp, Warning, TEXT("DeltaRotator is: %s"), *(DeltaRotator.ToString()))
		//UE_LOG(LogTemp, Warning, TEXT("DeltaRotator Yaw Normalized is: %f"), DeltaRotator.GetNormalized().Yaw)


}

bool UTankAimingComponent::IsBarrelMoving()
{
	if (!ensure(Barrel)) { return false; }
	return !(Barrel->GetForwardVector().Equals(AimDirection, .01f)); 
	
}



