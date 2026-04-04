#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "NexusGameplayAbility.h"
#include "GA_ShadowStrike.generated.h"

class UAbilityTask_PlayMontageAndWait;
class UAbilityTask_WaitDelay;
class ANexusCharacterBase;

UCLASS()
class NEXUS_API UGA_ShadowStrike : public UNexusGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_ShadowStrike();

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
	UPROPERTY(EditDefaultsOnly, Category="Shadow Strike")
	TObjectPtr<UAnimMontage> ShadowStrikeMontage;

	UPROPERTY(EditDefaultsOnly, Category="Shadow Strike")
	float TraceDistance = 5000.f;

	UPROPERTY(EditDefaultsOnly, Category="Shadow Strike")
	float Damage = 15.f;

	UPROPERTY(EditDefaultsOnly, Category="Shadow Strike")
	float TeleportDistanceBeyondTarget = 120.f;

	UPROPERTY(EditDefaultsOnly, Category="Shadow Strike")
	float UpOffset = 50.f;

	UPROPERTY(EditDefaultsOnly, Category="Shadow Strike")
	float StunDuration = 0.6f;

	UPROPERTY(EditDefaultsOnly, Category="Shadow Strike")
	bool bStopTargetMovement = true;

	UPROPERTY(EditDefaultsOnly, Category="Shadow Strike")
	float TeleportDelay = 0.15f;

	UPROPERTY(EditDefaultsOnly, Category="Shadow Strike")
	FGameplayTag ShadowStrikeCueTag;

	UPROPERTY(Transient)
	TObjectPtr<ANexusCharacterBase> CachedSourceCharacter;

	UPROPERTY(Transient)
	TObjectPtr<ANexusCharacterBase> CachedTargetCharacter;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_WaitDelay> TeleportDelayTask = nullptr;

	UPROPERTY(Transient)
	FVector CachedTeleportLocation = FVector::ZeroVector;

	UPROPERTY(Transient)
	bool bStrikeExecuted = false;

	UPROPERTY(Transient)
	bool bCueActive = false;

	bool FindTeleportLocation(FVector& OutLocation) const;
	void TeleportAndFaceTarget(const FVector& TeleportLocation);
	void StunTarget() const;
	void DamageTarget() const;

	void StartShadowStrikeCue();
	void StopShadowStrikeCue();
	void ExecuteShadowStrike();
	void Cleanup();

	UFUNCTION()
	void OnTeleportDelayFinished();

	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageInterrupted();

	UFUNCTION()
	void OnMontageCancelled();
};