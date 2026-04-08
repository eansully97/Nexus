#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Nexus/GameplayAbilitySystem/Abilities/NexusMontageTriggeredAbility.h"
#include "GA_LaunchProjectile.generated.h"

class ANexusProjectile;

UCLASS()
class NEXUS_API UGA_LaunchProjectile : public UNexusMontageTriggeredAbility
{
	GENERATED_BODY()

public:
	UGA_LaunchProjectile();

protected:
	virtual bool ExecuteTriggeredAction() override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Projectile")
	TSubclassOf<ANexusProjectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Projectile")
	float ProjectileSpeed = 2500.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Projectile")
	FName MuzzleSocketName = TEXT("Muzzle");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Projectile")
	float TraceDistance = 15000.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Projectile")
	float ProjectileDamage = 10.f;

	UFUNCTION(BlueprintCallable, Category="Projectile")
	bool SpawnProjectile();

	bool GetProjectileSpawnData(
		FVector& OutSpawnLocation,
		FRotator& OutSpawnRotation,
		FVector& OutShootDirection) const;

	bool GetAimHitLocation(FVector& OutHitLocation) const;
};