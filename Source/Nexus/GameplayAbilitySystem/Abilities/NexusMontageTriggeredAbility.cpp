// Fill out your copyright notice in the Description page of Project Settings.


#include "NexusMontageTriggeredAbility.h"
#include "Nexus/GameplayAbilitySystem/Abilities/NexusMontageTriggeredAbility.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Animation/AnimMontage.h"

UNexusMontageTriggeredAbility::UNexusMontageTriggeredAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UNexusMontageTriggeredAbility::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!ActorInfo || !ActorInfo->AvatarActor.IsValid())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (ShouldCommitAbilityOnActivate() && !CommitAbilityWithSetByCaller())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	bTriggeredActionExecuted = false;
	CleanupTasks();

	const bool bHasMontage = ShouldStartMontageTask();
	const bool bHasExecutionEvent = ShouldStartGameplayEventWaitTask();

	if (!bHasMontage)
	{
		const bool bSucceeded = ExecuteTriggeredAction();
		bTriggeredActionExecuted = bSucceeded;
		OnTriggeredActionExecuted(bSucceeded);

		if (!bSucceeded)
		{
			OnTriggeredActionFailed();
		}

		if (ShouldEndAbilityAfterImmediateExecution())
		{
			SafeEndAbility(!bSucceeded);
		}
		return;
	}

	if (bHasExecutionEvent)
	{
		StartGameplayEventWaitTask();
	}
	else
	{
		const bool bSucceeded = ExecuteTriggeredAction();
		bTriggeredActionExecuted = bSucceeded;
		OnTriggeredActionExecuted(bSucceeded);

		if (!bSucceeded)
		{
			OnTriggeredActionFailed();
		}
	}

	StartMontageTask();

	if (!ActiveMontageTask)
	{
		SafeEndAbility(true);
	}
}

void UNexusMontageTriggeredAbility::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	CleanupTasks();
	bTriggeredActionExecuted = false;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

bool UNexusMontageTriggeredAbility::ShouldStartMontageTask() const
{
	return IsValid(MontageToPlay);
}

bool UNexusMontageTriggeredAbility::ShouldStartGameplayEventWaitTask() const
{
	return ExecutionEventTag.IsValid();
}

void UNexusMontageTriggeredAbility::StartMontageTask()
{
	if (!IsValid(MontageToPlay))
	{
		return;
	}

	ActiveMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		TEXT("PlayMontageTriggeredAbilityMontage"),
		MontageToPlay,
		MontagePlayRate,
		MontageStartSection,
		bStopMontageWhenAbilityEnds,
		AnimRootMotionTranslationScale,
		MontageStartTimeSeconds,
		bAllowInterruptAfterBlendOut);

	if (!ActiveMontageTask)
	{
		return;
	}

	ActiveMontageTask->OnCompleted.AddDynamic(this, &ThisClass::HandleMontageCompleted);
	ActiveMontageTask->OnInterrupted.AddDynamic(this, &ThisClass::HandleMontageInterrupted);
	ActiveMontageTask->OnCancelled.AddDynamic(this, &ThisClass::HandleMontageCancelled);
	ActiveMontageTask->ReadyForActivation();
}

void UNexusMontageTriggeredAbility::StartGameplayEventWaitTask()
{
	if (!ExecutionEventTag.IsValid())
	{
		return;
	}

	ActiveWaitGameplayEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		ExecutionEventTag,
		nullptr,
		bOnlyTriggerExecutionEventOnce,
		bOnlyMatchExactExecutionEventTag);

	if (!ActiveWaitGameplayEventTask)
	{
		return;
	}

	ActiveWaitGameplayEventTask->EventReceived.AddDynamic(
		this,
		&ThisClass::HandleExecutionGameplayEvent);
	ActiveWaitGameplayEventTask->ReadyForActivation();
}

void UNexusMontageTriggeredAbility::CleanupTasks()
{
	if (ActiveWaitGameplayEventTask)
	{
		ActiveWaitGameplayEventTask->EndTask();
		ActiveWaitGameplayEventTask = nullptr;
	}

	if (ActiveMontageTask)
	{
		ActiveMontageTask->EndTask();
		ActiveMontageTask = nullptr;
	}
}

void UNexusMontageTriggeredAbility::SafeEndAbility(bool bWasCancelled)
{
	if (!IsActive())
	{
		return;
	}

	if (!CurrentActorInfo)
	{
		return;
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, bWasCancelled);
}

void UNexusMontageTriggeredAbility::HandleExecutionGameplayEvent(FGameplayEventData Payload)
{
	if (bTriggeredActionExecuted)
	{
		return;
	}

	const bool bSucceeded = ExecuteTriggeredAction();
	bTriggeredActionExecuted = bSucceeded;
	OnTriggeredActionExecuted(bSucceeded);

	if (!bSucceeded)
	{
		OnTriggeredActionFailed();
	}
}

void UNexusMontageTriggeredAbility::HandleMontageCompleted()
{
	if (!bTriggeredActionExecuted && bFallbackExecuteOnMontageCompleted)
	{
		const bool bSucceeded = ExecuteTriggeredAction();
		bTriggeredActionExecuted = bSucceeded;
		OnTriggeredActionExecuted(bSucceeded);

		if (!bSucceeded)
		{
			OnTriggeredActionFailed();
		}
	}

	SafeEndAbility(false);
}

void UNexusMontageTriggeredAbility::HandleMontageInterrupted()
{
	SafeEndAbility(true);
}

void UNexusMontageTriggeredAbility::HandleMontageCancelled()
{
	SafeEndAbility(true);
}