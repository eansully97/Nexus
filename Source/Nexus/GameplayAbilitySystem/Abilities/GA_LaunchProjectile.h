#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "NexusGameplayAbility.h"
#include "GA_LaunchProjectile.generated.h"

class ANexusProjectile;
class UAbilityTask_PlayMontageAndWait;
class UAbilityTask_WaitGameplayEvent;
class UAnimMontage;

UCLASS()
class NEXUS_API UGA_LaunchProjectile : public UNexusGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_LaunchProjectile();

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

protected:
	/* ------------------------------ Animation / Event ------------------------------ */

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animation")
	TObjectPtr<UAnimMontage> MontageToPlay = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animation")
	float MontagePlayRate = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animation")
	FName MontageStartSection = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animation")
	bool bStopMontageWhenAbilityEnds = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animation")
	float AnimRootMotionTranslationScale = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animation")
	float MontageStartTimeSeconds = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animation")
	bool bAllowInterruptAfterBlendOut = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animation")
	FGameplayTag SpawnProjectileEventTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animation")
	bool bOnlyTriggerSpawnEventOnce = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animation")
	bool bOnlyMatchExactSpawnEventTag = true;

	/* -------------------------------- Projectile ---------------------------------- */

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

protected:
	UFUNCTION()
	void HandleSpawnProjectileEvent(FGameplayEventData Payload);

	UFUNCTION()
	void HandleMontageCompleted();

	UFUNCTION()
	void HandleMontageInterrupted();

	UFUNCTION()
	void HandleMontageCancelled();

	void StartMontageTask();
	void StartSpawnProjectileWaitTask();
	void CleanupTasks();
	void SafeEndAbility(bool bWasCancelled);

protected:
	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_PlayMontageAndWait> ActiveMontageTask = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_WaitGameplayEvent> ActiveWaitGameplayEventTask = nullptr;

	UPROPERTY(Transient)
	bool bProjectileSpawned = false;
};