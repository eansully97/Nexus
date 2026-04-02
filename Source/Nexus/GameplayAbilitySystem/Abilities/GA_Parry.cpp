#include "GA_Parry.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayCueFunctionLibrary.h"
#include "Nexus/Character/NexusCharacterBase.h"
#include "Nexus/NexusGameplayTags.h"

UGA_Parry::UGA_Parry()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(NexusGameplayTags::Ability_Reactive_Parry);
	SetAssetTags(AssetTags);

	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag = NexusGameplayTags::Event_Parry_Activated;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);
}

bool UGA_Parry::TryExtractParryActors(
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayEventData* TriggerEventData,
	ANexusCharacterBase*& OutDefender,
	ANexusCharacterBase*& OutAttacker) const
{
	OutDefender = nullptr;
	OutAttacker = nullptr;

	if (!ActorInfo || !ActorInfo->AvatarActor.IsValid() || !TriggerEventData)
	{
		return false;
	}

	OutDefender = Cast<ANexusCharacterBase>(ActorInfo->AvatarActor.Get());
	OutAttacker = Cast<ANexusCharacterBase>(const_cast<AActor*>(TriggerEventData->Instigator.Get()));

	if (!IsValid(OutDefender) || !IsValid(OutAttacker) || OutDefender == OutAttacker)
	{
		return false;
	}

	if (!OutDefender->HasAuthority())
	{
		return false;
	}

	return true;
}

void UGA_Parry::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	ANexusCharacterBase* Defender = nullptr;
	ANexusCharacterBase* Attacker = nullptr;

	if (!TryExtractParryActors(ActorInfo, TriggerEventData, Defender, Attacker))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (TriggerEventData)
	{
		BP_OnParryActivated(*TriggerEventData);
	}

	HandleSuccessfulParry(Defender, Attacker, TriggerEventData);

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UGA_Parry::HandleSuccessfulParry(
	ANexusCharacterBase* Defender,
	ANexusCharacterBase* Attacker,
	const FGameplayEventData* TriggerEventData) const
{
	if (!IsValid(Defender) || !IsValid(Attacker) || !TriggerEventData || !Defender->HasAuthority())
	{
		return;
	}

	Defender->ApplyStunToTarget(Attacker, StunDuration);

	if (bPlayParryCueOnDefender)
	{
		FGameplayCueParameters DefenderCueParams;
		DefenderCueParams.Instigator = Attacker;
		DefenderCueParams.EffectCauser = Attacker;

		UGameplayCueFunctionLibrary::ExecuteGameplayCueOnActor(
			Defender,
			NexusGameplayTags::GameplayCue_Damage_Deflected,
			DefenderCueParams
		);
	}

	if (bPlayParryCueOnAttacker)
	{
		FGameplayCueParameters AttackerCueParams;
		AttackerCueParams.Instigator = Defender;
		AttackerCueParams.EffectCauser = Defender;

		UGameplayCueFunctionLibrary::ExecuteGameplayCueOnActor(
			Attacker,
			NexusGameplayTags::GameplayCue_Damage_Deflected,
			AttackerCueParams
		);
	}
}