#pragma once

#include "CoreMinimal.h"
#include "Nexus/GameplayAbilitySystem/Abilities/NexusGameplayAbility.h"
#include "Nexus/FunctionLibraries/NexusAbilityFunctionLibrary.h"
#include "GA_ArcanePulse.generated.h"

class UAbilityTask_PlayMontageAndWait;
class UAbilityTask_WaitDelay;
class UAnimMontage;

UCLASS()
class NEXUS_API UGA_ArcanePulse : public UNexusGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_ArcanePulse();

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

	void CleanupTasks();
	void ExecuteArcanePulse();

	UFUNCTION()
	void OnEffectDelayFinished();

	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageInterrupted();

	UFUNCTION()
	void OnMontageCancelled();

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Arcane Pulse")
	TObjectPtr<UAnimMontage> MontageToPlay = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Arcane Pulse")
	float MontagePlayRate = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Arcane Pulse")
	FName MontageStartSection = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Arcane Pulse")
	bool bStopMontageWhenAbilityEnds = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Arcane Pulse")
	float AnimRootMotionTranslationScale = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Arcane Pulse")
	float MontageStartTimeSeconds = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Arcane Pulse")
	bool bAllowInterruptAfterBlendOut = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Arcane Pulse")
	float PulseRadius = 450.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Arcane Pulse")
	float Damage = 20.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Arcane Pulse")
	float StunDuration = 0.4f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Arcane Pulse")
	float EffectDelay = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Arcane Pulse")
	bool bRequireLineOfSight = false;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_WaitDelay> EffectDelayTask = nullptr;

	UPROPERTY(Transient)
	bool bPulseExecuted = false;
};
