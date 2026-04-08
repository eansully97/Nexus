#include "GA_Enrage.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Animation/AnimMontage.h"
#include "Nexus/NexusGameplayTags.h"
#include "Nexus/Character/NexusCharacterBase.h"

UGA_Enrage::UGA_Enrage()
{
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(NexusGameplayTags::Ability_Buff_Enrage);
	SetAssetTags(AssetTags);
	ActivationOwnedTags.AddTag(NexusGameplayTags::Status_Ability_Active);
}

void UGA_Enrage::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	MontageTask = nullptr;

	if (!ActorInfo || !ActorInfo->AvatarActor.IsValid() || !IsValid(GetNexusCharacterFromActorInfo()))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbilityWithSetByCaller())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ApplyEnrageEffect();

	if (!MontageToPlay)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
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

void UGA_Enrage::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (MontageTask)
	{
		MontageTask->EndTask();
		MontageTask = nullptr;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_Enrage::ApplyEnrageEffect() const
{
	ANexusCharacterBase* SourceCharacter = GetNexusCharacterFromActorInfo();
	if (!IsValid(SourceCharacter) || HealAmount <= 0.0f)
	{
		return;
	}

	if (AbilityDuration > 0.0f)
	{
		SourceCharacter->ApplyHealToTargetWithDuration(SourceCharacter, HealAmount, AbilityDuration);
		return;
	}

	SourceCharacter->ApplyHealToTarget(SourceCharacter, HealAmount);
}

void UGA_Enrage::OnMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_Enrage::OnMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UGA_Enrage::OnMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}
