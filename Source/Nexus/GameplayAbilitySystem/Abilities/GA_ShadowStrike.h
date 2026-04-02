#pragma once

#include "CoreMinimal.h"
#include "NexusGameplayAbility.h"
#include "GA_ShadowStrike.generated.h"


class UAbilityTask_PlayMontageAndWait;
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
	float MaxRange = 1500.f;

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

	UPROPERTY(Transient)
	TObjectPtr<ANexusCharacterBase> CachedSourceCharacter;

	UPROPERTY(Transient)
	TObjectPtr<ANexusCharacterBase> CachedTargetCharacter;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;

	UPROPERTY(Transient)
	bool bCommittedSuccessfully = false;

	ANexusCharacterBase* GetSourceCharacter() const;

	bool IsValidTarget(ANexusCharacterBase* SourceCharacter, ANexusCharacterBase* TargetCharacter) const;
	bool IsInRange(ANexusCharacterBase* SourceCharacter, ANexusCharacterBase* TargetCharacter) const;
	bool FindTeleportLocation(FVector& OutLocation, FRotator& OutRotation) const;
	void TeleportAndFaceTarget(const FVector& TeleportLocation);

	void StunTarget() const;
	void DamageTarget() const;
	void Cleanup();

	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageInterrupted();

	UFUNCTION()
	void OnMontageCancelled();
};