#include "GA_NexusParry.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayCueFunctionLibrary.h"
#include "Nexus/Character/NexusCharacterBase.h"
#include "Nexus/NexusGameplayTags.h"

UGA_NexusParry::UGA_NexusParry()
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

void UGA_NexusParry::ActivateAbility(
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

	ANexusCharacterBase* Defender = Cast<ANexusCharacterBase>(ActorInfo->AvatarActor.Get());
	if (!IsValid(Defender) || !Defender->HasAuthority())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const bool bCommitted = CommitAbility(Handle, ActorInfo, ActivationInfo);

	UE_LOG(LogTemp, Warning,
		TEXT("GA_NexusParry CommitAbility=%d"),
		bCommitted ? 1 : 0);

	if (!bCommitted)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}



	HandleSuccessfulParry(TriggerEventData);

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UGA_NexusParry::HandleSuccessfulParry(const FGameplayEventData* TriggerEventData) const
{
	ANexusCharacterBase* Defender = Cast<ANexusCharacterBase>(GetAvatarActorFromActorInfo());
	if (!IsValid(Defender) || !TriggerEventData || !Defender->HasAuthority())
	{
		return;
	}

	ANexusCharacterBase* Attacker = Cast<ANexusCharacterBase>(const_cast<AActor*>(TriggerEventData->Instigator.Get()));
	if (!IsValid(Attacker) || Attacker == Defender)
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