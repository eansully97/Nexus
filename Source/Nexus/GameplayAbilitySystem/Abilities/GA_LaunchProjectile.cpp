#include "GA_LaunchProjectile.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimMontage.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Nexus/Character/Player/NexusPlayerCharacter.h"
#include "Nexus/Components/NexusWeaponsManager.h"
#include "Nexus/Projectiles/NexusProjectile.h"
#include "Nexus/Weapons/NexusWeaponBase.h"

DEFINE_LOG_CATEGORY_STATIC(LogGALaunchProjectile, Log, All);

UGA_LaunchProjectile::UGA_LaunchProjectile()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	ExecutionEventTag = FGameplayTag();
}

bool UGA_LaunchProjectile::ExecuteTriggeredAction()
{
	return SpawnProjectile();
}

bool UGA_LaunchProjectile::SpawnProjectile()
{
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	if (!ActorInfo || !ActorInfo->IsNetAuthority())
	{
		return false;
	}

	if (!ProjectileClass)
	{
		UE_LOG(LogGALaunchProjectile, Warning, TEXT("LaunchProjectile: ProjectileClass is null."));
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogGALaunchProjectile, Warning, TEXT("LaunchProjectile: World is null."));
		return false;
	}

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor))
	{
		UE_LOG(LogGALaunchProjectile, Warning, TEXT("LaunchProjectile: AvatarActor is invalid."));
		return false;
	}

	FVector SpawnLocation = FVector::ZeroVector;
	FRotator SpawnRotation = FRotator::ZeroRotator;
	FVector ShootDirection = FVector::ForwardVector;

	if (!GetProjectileSpawnData(SpawnLocation, SpawnRotation, ShootDirection))
	{
		UE_LOG(LogGALaunchProjectile, Warning, TEXT("LaunchProjectile: Failed to resolve spawn data."));
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = AvatarActor;
	SpawnParams.Instigator = Cast<APawn>(AvatarActor);
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ANexusProjectile* Projectile = World->SpawnActorDeferred<ANexusProjectile>(
		ProjectileClass,
		FTransform(SpawnRotation, SpawnLocation),
		AvatarActor,
		SpawnParams.Instigator,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

	if (!Projectile)
	{
		UE_LOG(LogGALaunchProjectile, Warning, TEXT("LaunchProjectile: SpawnActorDeferred failed."));
		return false;
	}

	Projectile->InitializeProjectile(
		AvatarActor,
		GetAbilitySystemComponentFromActorInfo(),
		ShootDirection,
		ProjectileSpeed,
		ProjectileDamage);

	UGameplayStatics::FinishSpawningActor(
		Projectile,
		FTransform(SpawnRotation, SpawnLocation));

	return true;
}

bool UGA_LaunchProjectile::GetProjectileSpawnData(
	FVector& OutSpawnLocation,
	FRotator& OutSpawnRotation,
	FVector& OutShootDirection) const
{
	OutSpawnLocation = FVector::ZeroVector;
	OutSpawnRotation = FRotator::ZeroRotator;
	OutShootDirection = FVector::ForwardVector;

	const ANexusPlayerCharacter* Character = GetNexusPlayerCharacterFromActorInfo();
	if (!IsValid(Character))
	{
		return false;
	}

	const UNexusWeaponsManager* WeaponsManager = Character->GetWeaponsManager();
	if (!IsValid(WeaponsManager))
	{
		return false;
	}

	const ANexusWeaponBase* WeaponFiring = WeaponsManager->EquippedWeapon;
	if (!IsValid(WeaponFiring))
	{
		return false;
	}

	const USceneComponent* WeaponMesh = WeaponFiring->GetWeaponMesh();
	if (!IsValid(WeaponMesh))
	{
		return false;
	}

	if (MuzzleSocketName != NAME_None && WeaponMesh->DoesSocketExist(MuzzleSocketName))
	{
		OutSpawnLocation = WeaponMesh->GetSocketLocation(MuzzleSocketName);
	}
	else
	{
		OutSpawnLocation = WeaponMesh->GetComponentLocation();
	}

	FVector AimLocation = FVector::ZeroVector;
	if (!GetAimHitLocation(AimLocation))
	{
		const AController* Controller = Character->GetController();
		const FRotator FallbackRotation = IsValid(Controller)
			? Controller->GetControlRotation()
			: Character->GetActorRotation();

		AimLocation = OutSpawnLocation + (FallbackRotation.Vector() * TraceDistance);
	}

	OutShootDirection = (AimLocation - OutSpawnLocation).GetSafeNormal();

	if (OutShootDirection.IsNearlyZero())
	{
		OutShootDirection = Character->GetActorForwardVector();
	}

	OutSpawnRotation = OutShootDirection.Rotation();
	return true;
}

bool UGA_LaunchProjectile::GetAimHitLocation(FVector& OutHitLocation) const
{
	OutHitLocation = FVector::ZeroVector;

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	APawn* Pawn = Cast<APawn>(AvatarActor);
	if (!IsValid(Pawn))
	{
		return false;
	}

	AController* Controller = Pawn->GetController();
	if (!IsValid(Controller))
	{
		return false;
	}

	FVector ViewLocation = FVector::ZeroVector;
	FRotator ViewRotation = FRotator::ZeroRotator;
	Controller->GetPlayerViewPoint(ViewLocation, ViewRotation);

	const FVector TraceStart = ViewLocation;
	const FVector TraceEnd = TraceStart + (ViewRotation.Vector() * TraceDistance);

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ProjectileAimTrace), false, Pawn);
	Params.AddIgnoredActor(Pawn);

	if (IsValid(AvatarActor))
	{
		Params.AddIgnoredActor(AvatarActor);
	}

	const bool bHit = World->LineTraceSingleByChannel(
		Hit,
		TraceStart,
		TraceEnd,
		ECC_Visibility,
		Params);

	OutHitLocation = bHit ? Hit.ImpactPoint : TraceEnd;
	return true;
}