// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NexusGameplayAbility.h"
#include "ShadowStrike.generated.h"

class UAbilityTask_WaitGameplayEvent;
class UAbilityTask_PlayMontageAndWait;
class ANexusPlayerController;
class ANexusCharacterBase;
/**
 * 
 */
UCLASS()
class NEXUS_API UShadowStrike : public UNexusGameplayAbility
{
	GENERATED_BODY()
	
public:
	UShadowStrike();

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
	
	UPROPERTY(EditDefaultsOnly, Category = "Shadow Strike")
	TObjectPtr<UAnimMontage> ShadowStrikeMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Shadow Strike")
	float TraceDistance = 5000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Shadow Strike")
	float MaxRange = 2000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Shadow Strike")
	float TeleportDistanceBehindTarget = 100.f;

	UPROPERTY(EditDefaultsOnly, Category = "Shadow Strike")
	float SideOffset = 50.f;

	UPROPERTY(EditDefaultsOnly, Category = "Shadow Strike")
	bool bStopTargetMovement = true;

	UPROPERTY()
	TObjectPtr<ANexusCharacterBase> CachedSourceCharacter;

	UPROPERTY()
	TObjectPtr<ANexusCharacterBase> CachedTargetCharacter;

	UPROPERTY()
	TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;
	
	ANexusCharacterBase* GetSourceCharacter() const;
	
	bool IsValidTarget(ANexusCharacterBase* SourceCharacter, ANexusCharacterBase* TargetCharacter) const;
	bool IsInRange(ANexusCharacterBase* SourceCharacter, ANexusCharacterBase* TargetCharacter) const;
	bool FindTeleportLocation(FVector& OutLocation, FRotator& OutRotation) const;
	void TeleportAndFaceTarget(const FVector& TeleportLocation);

	void StunTarget() const;
	void Cleanup();

	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageInterrupted();

	UFUNCTION()
	void OnMontageCancelled();
};
