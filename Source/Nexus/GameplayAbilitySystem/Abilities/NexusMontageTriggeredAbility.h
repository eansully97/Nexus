#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Nexus/GameplayAbilitySystem/Abilities/NexusGameplayAbility.h"
#include "NexusMontageTriggeredAbility.generated.h"

class UAbilityTask_PlayMontageAndWait;
class UAbilityTask_WaitGameplayEvent;
class UAnimMontage;

UCLASS(Abstract)
class NEXUS_API UNexusMontageTriggeredAbility : public UNexusGameplayAbility
{
	GENERATED_BODY()

public:
	UNexusMontageTriggeredAbility();

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
	FGameplayTag ExecutionEventTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animation")
	bool bOnlyTriggerExecutionEventOnce = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animation")
	bool bOnlyMatchExactExecutionEventTag = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animation")
	bool bFallbackExecuteOnMontageCompleted = false;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_PlayMontageAndWait> ActiveMontageTask = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_WaitGameplayEvent> ActiveWaitGameplayEventTask = nullptr;

	UPROPERTY(Transient)
	bool bTriggeredActionExecuted = false;

protected:
	virtual bool ExecuteTriggeredAction() PURE_VIRTUAL(UNexusMontageTriggeredAbility::ExecuteTriggeredAction, return false;);

	virtual bool ShouldCommitAbilityOnActivate() const { return true; }
	virtual bool ShouldEndAbilityAfterImmediateExecution() const { return true; }
	virtual bool ShouldStartMontageTask() const;
	virtual bool ShouldStartGameplayEventWaitTask() const;
	virtual void OnTriggeredActionExecuted(bool bSucceeded) {}
	virtual void OnTriggeredActionFailed() {}

	void StartMontageTask();
	void StartGameplayEventWaitTask();
	void CleanupTasks();
	void SafeEndAbility(bool bWasCancelled);

	UFUNCTION()
	void HandleExecutionGameplayEvent(FGameplayEventData Payload);

	UFUNCTION()
	void HandleMontageCompleted();

	UFUNCTION()
	void HandleMontageInterrupted();

	UFUNCTION()
	void HandleMontageCancelled();
};