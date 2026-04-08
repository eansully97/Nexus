#include "GA_ArcanePulse.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Animation/AnimMontage.h"
#include "Nexus/NexusGameplayTags.h"
#include "Nexus/Character/NexusCharacterBase.h"

UGA_ArcanePulse::UGA_ArcanePulse()
{
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(NexusGameplayTags::Ability_Damage_AOE_ArcanePulse);
	SetAssetTags(AssetTags);
	ActivationOwnedTags.AddTag(NexusGameplayTags::Status_Ability_Active);
	ActivateGameplayCueTag = NexusGameplayTags::GameplayCue_Ability_ArcanePulse_Activate;
}

void UGA_ArcanePulse::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	CleanupTasks();
	bPulseExecuted = false;

	ANexusCharacterBase* SourceCharacter = GetNexusCharacterFromActorInfo();
	if (!ActorInfo || !ActorInfo->AvatarActor.IsValid() || !IsValid(SourceCharacter))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbilityWithSetByCaller())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (EffectDelay > 0.0f)
	{
		EffectDelayTask = UAbilityTask_WaitDelay::WaitDelay(this, EffectDelay);
		if (EffectDelayTask)
		{
			EffectDelayTask->OnFinish.AddDynamic(this, &ThisClass::OnEffectDelayFinished);
			EffectDelayTask->ReadyForActivation();
		}
	}
	else
	{
		ExecuteArcanePulse();
	}

	if (!MontageToPlay)
	{
		if (!EffectDelayTask)
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

void UGA_ArcanePulse::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	CleanupTasks();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_ArcanePulse::CleanupTasks()
{
	if (EffectDelayTask)
	{
		EffectDelayTask->EndTask();
		EffectDelayTask = nullptr;
	}

	if (MontageTask)
	{
		MontageTask->EndTask();
		MontageTask = nullptr;
	}
}

void UGA_ArcanePulse::ExecuteArcanePulse()
{
	if (bPulseExecuted)
	{
		return;
	}

	ANexusCharacterBase* SourceCharacter = GetNexusCharacterFromActorInfo();
	if (!IsValid(SourceCharacter))
	{
		return;
	}

	bPulseExecuted = true;

	if (!CurrentActorInfo || !CurrentActorInfo->IsNetAuthority())
	{
		return;
	}

	TArray<FNexusAbilityTargetHit> TargetHits;
	const FVector PulseOrigin = SourceCharacter->GetActorLocation();

	if (!UNexusAbilityFunctionLibrary::QueryRadialEnemyCharacterTargets(
			this,
			PulseOrigin,
			PulseRadius,
			SourceCharacter,
			TargetHits,
			bRequireLineOfSight))
	{
		return;
	}

	for (const FNexusAbilityTargetHit& TargetHit : TargetHits)
	{
		ANexusCharacterBase* TargetCharacter = Cast<ANexusCharacterBase>(TargetHit.TargetActor);
		if (!IsValid(TargetCharacter))
		{
			continue;
		}

		SourceCharacter->ApplyDamageToTargetWithCueParams(
			TargetCharacter,
			Damage,
			SourceCharacter,
			SourceCharacter,
			&TargetHit.HitResult,
			this);

		if (StunDuration > 0.0f)
		{
			SourceCharacter->ApplyStunToTarget(TargetCharacter, StunDuration);
		}
	}
}

void UGA_ArcanePulse::OnEffectDelayFinished()
{
	EffectDelayTask = nullptr;
	ExecuteArcanePulse();

	if (!MontageTask)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void UGA_ArcanePulse::OnMontageCompleted()
{
	MontageTask = nullptr;

	if (EffectDelayTask)
	{
		return;
	}

	if (!bPulseExecuted)
	{
		ExecuteArcanePulse();
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_ArcanePulse::OnMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UGA_ArcanePulse::OnMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}
