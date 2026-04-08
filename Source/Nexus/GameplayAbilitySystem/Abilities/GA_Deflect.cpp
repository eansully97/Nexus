#include "GA_Deflect.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Animation/AnimMontage.h"
#include "Nexus/NexusGameplayTags.h"
#include "Nexus/GameplayAbilitySystem/AbilitySystemComponent/NexusAbilitySystemComponent.h"

UGA_Deflect::UGA_Deflect()
{
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(NexusGameplayTags::Ability_Weapon_Axe_Deflect);
	SetAssetTags(AssetTags);
	ActivationOwnedTags.AddTag(NexusGameplayTags::Status_Ability_Active);
	DeflectStatusTag = NexusGameplayTags::Status_Defense_Deflecting;
}

void UGA_Deflect::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	CleanupTasks();
	bDeflectStateApplied = false;

	if (!ActorInfo || !ActorInfo->AvatarActor.IsValid())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbilityWithSetByCaller())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ApplyDeflectState(true);

	if (AbilityDuration > 0.0f)
	{
		DurationTask = UAbilityTask_WaitDelay::WaitDelay(this, AbilityDuration);
		if (DurationTask)
		{
			DurationTask->OnFinish.AddDynamic(this, &ThisClass::OnDurationFinished);
			DurationTask->ReadyForActivation();
		}
	}

	if (!MontageToPlay)
	{
		if (!DurationTask)
		{
			EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		}
		return;
	}

	MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		NAME_None,
		MontageToPlay,
		MontagePlayRate,
		MontageStartSection,
		bStopMontageWhenAbilityEnds,
		AnimRootMotionTranslationScale,
		MontageStartTimeSeconds,
		bAllowInterruptAfterBlendOut);

	if (!MontageTask)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageCompleted);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageCancelled);
	MontageTask->ReadyForActivation();
}

void UGA_Deflect::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	ApplyDeflectState(false);
	CleanupTasks();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_Deflect::CleanupTasks()
{
	if (DurationTask)
	{
		DurationTask->EndTask();
		DurationTask = nullptr;
	}

	if (MontageTask)
	{
		MontageTask->EndTask();
		MontageTask = nullptr;
	}
}

void UGA_Deflect::ApplyDeflectState(bool bShouldApply)
{
	UNexusAbilitySystemComponent* NexusASC = GetNexusAbilitySystemComponentFromActorInfo();
	if (!NexusASC || !DeflectStatusTag.IsValid())
	{
		return;
	}

	if (bShouldApply)
	{
		if (!bDeflectStateApplied)
		{
			NexusASC->AddLooseGameplayTag(DeflectStatusTag);
			bDeflectStateApplied = true;
		}

		return;
	}

	if (bDeflectStateApplied)
	{
		NexusASC->RemoveLooseGameplayTag(DeflectStatusTag);
		bDeflectStateApplied = false;
	}
}

void UGA_Deflect::OnDurationFinished()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_Deflect::OnMontageCompleted()
{
	MontageTask = nullptr;

	if (!DurationTask)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void UGA_Deflect::OnMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UGA_Deflect::OnMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}
