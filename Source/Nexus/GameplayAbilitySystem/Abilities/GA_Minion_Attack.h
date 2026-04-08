#pragma once

#include "CoreMinimal.h"
#include "Nexus/GameplayAbilitySystem/Abilities/NexusGameplayAbility.h"
#include "GA_Minion_Attack.generated.h"

class ANexusMinionBase;
class UAbilityTask_PlayMontageAndWait;
class UAbilityTask_WaitGameplayEvent;
class UAnimMontage;

UCLASS()
class NEXUS_API UGA_Minion_Attack : public UNexusGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Minion_Attack();

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
	void EndHitscanWindow();
	bool TryResolveMinion(ANexusMinionBase*& OutMinion) const;

	UFUNCTION()
	void OnHitScanStart(FGameplayEventData Payload);

	UFUNCTION()
	void OnHitScanEnd(FGameplayEventData Payload);

	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageInterrupted();

	UFUNCTION()
	void OnMontageCancelled();

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Attack")
	TObjectPtr<UAnimMontage> MontageToPlay = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Attack")
	float MontagePlayRate = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Attack")
	FName MontageStartSection = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Attack")
	bool bStopMontageWhenAbilityEnds = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Attack")
	float AnimRootMotionTranslationScale = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Attack")
	float MontageStartTimeSeconds = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Attack")
	bool bAllowInterruptAfterBlendOut = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Attack")
	FGameplayTag HitScanStartEventTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Attack")
	FGameplayTag HitScanEndEventTag;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_WaitGameplayEvent> StartHitScanTask = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_WaitGameplayEvent> EndHitScanTask = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<ANexusMinionBase> CachedMinion = nullptr;

	UPROPERTY(Transient)
	bool bHitscanWindowOpen = false;
};
