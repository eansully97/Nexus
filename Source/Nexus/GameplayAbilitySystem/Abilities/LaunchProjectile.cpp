// NexusGameplayAbility_FireProjectile.cpp


#include "LaunchProjectile.h"

#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "AbilitySystemComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Nexus/Character/Player/NexusPlayerCharacter.h"
#include "Nexus/Components/NexusWeaponsManager.h"
#include "Nexus/Controller/NexusPlayerController.h"
#include "Nexus/Projectiles/NexusProjectile.h"

ULaunchProjectile::ULaunchProjectile()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

bool ULaunchProjectile::SpawnProjectile()
{
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	if (!ActorInfo || !ActorInfo->IsNetAuthority())
	{
		return false;
	}
	
	if (!ProjectileClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("FireProjectile Ability: ProjectileClass is null"));
		return false;
	}

	FVector SpawnLocation = FVector::ZeroVector;
	FRotator SpawnRotation = FRotator::ZeroRotator;
	FVector ShootDirection = FVector::ForwardVector;

	if (!GetProjectileSpawnData(SpawnLocation, SpawnRotation, ShootDirection))
	{
		UE_LOG(LogTemp, Warning, TEXT("FireProjectile Ability: Failed to get projectile spawn data"));
		return false;
	}

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!AvatarActor)
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = AvatarActor;
	SpawnParams.Instigator = Cast<APawn>(AvatarActor);
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	ANexusProjectile* Projectile = World->SpawnActorDeferred<ANexusProjectile>(
		ProjectileClass,
		FTransform(SpawnRotation, SpawnLocation),
		AvatarActor,
		SpawnParams.Instigator,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn
	);

	if (!Projectile)
	{
		UE_LOG(LogTemp, Warning, TEXT("FireProjectile Ability: SpawnActorDeferred failed"));
		return false;
	}
	
	Projectile->InitializeProjectile(
		AvatarActor,
		GetAbilitySystemComponentFromActorInfo(),
		ShootDirection,
		ProjectileSpeed,
		ProjectileDamage
	);

	UGameplayStatics::FinishSpawningActor(Projectile, FTransform(SpawnRotation, SpawnLocation));

	return true;
}

bool ULaunchProjectile::GetProjectileSpawnData(
	FVector& OutSpawnLocation,
	FRotator& OutSpawnRotation,
	FVector& OutShootDirection) const
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	ANexusPlayerCharacter* Character = Cast<ANexusPlayerCharacter>(AvatarActor);
	ANexusWeaponBase* WeaponFiring = Character->GetWeaponsManager()->EquippedWeapon;
	if (!WeaponFiring)
	{
		return false;
	}

	const UStaticMeshComponent* Mesh = WeaponFiring->GetWeaponMesh();
	if (!Mesh)
	{
		return false;
	}

	OutSpawnLocation = Mesh->GetSocketLocation(MuzzleSocketName);

	FVector AimLocation = FVector::ZeroVector;
	if (!GetAimHitLocation(AimLocation))
	{
		// fallback: shoot forward from controller view if trace fails
		const FRotator ControlRot = Character->GetControlRotation();
		AimLocation = OutSpawnLocation + (ControlRot.Vector() * TraceDistance);
	}

	OutShootDirection = (AimLocation - OutSpawnLocation).GetSafeNormal();
	if (OutShootDirection.IsNearlyZero())
	{
		OutShootDirection = Character->GetActorForwardVector();
	}

	OutSpawnRotation = OutShootDirection.Rotation();
	return true;
}

bool ULaunchProjectile::GetAimHitLocation(FVector& OutHitLocation) const
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	APawn* Pawn = Cast<APawn>(AvatarActor);
	if (!Pawn)
	{
		return false;
	}

	AController* Controller = Pawn->GetController();
	if (!Controller)
	{
		return false;
	}

	FVector ViewLocation;
	FRotator ViewRotation;

	Controller->GetPlayerViewPoint(ViewLocation, ViewRotation);

	const FVector TraceStart = ViewLocation;
	const FVector TraceEnd = TraceStart + (ViewRotation.Vector() * TraceDistance);

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ProjectileAimTrace), false, Pawn);
	Params.AddIgnoredActor(Pawn);

	if (AvatarActor)
	{
		Params.AddIgnoredActor(AvatarActor);
	}

	const bool bHit = GetWorld()->LineTraceSingleByChannel(
		Hit,
		TraceStart,
		TraceEnd,
		ECC_Visibility,
		Params
	);

	OutHitLocation = bHit ? Hit.ImpactPoint : TraceEnd;

	return true;
}