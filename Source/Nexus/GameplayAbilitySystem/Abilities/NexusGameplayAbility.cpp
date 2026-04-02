// Fill out your copyright notice in the Description page of Project Settings.


// ReSharper disable CppExpressionWithoutSideEffects
#include "NexusGameplayAbility.h"
#include "Nexus/NexusEnumTypes.h"
#include "Nexus/Character/NexusCharacterBase.h"
#include "Nexus/DataAssets/AbilityInfo.h"
#include "Nexus/DataAssets/CostAndCooldownConfig.h"

UNexusGameplayAbility::UNexusGameplayAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

FText UNexusGameplayAbility::GetAbilityDisplayName() const
{
	return AbilityInfo ? AbilityInfo->AbilityDisplayName : FText();
}

EAbilityContainerInfo UNexusGameplayAbility::GetContainerToShowIn() const
{
	return AbilityInfo ? AbilityInfo->ContainerToShowIn : EAbilityContainerInfo::None;
}

ANexusCharacterBase* UNexusGameplayAbility::GetNexusCharacterFromActorInfo() const
{
	return Cast<ANexusCharacterBase>(GetAvatarActorFromActorInfo());
}

UAbilitySystemComponent* UNexusGameplayAbility::GetSourceAbilitySystemComponent() const
{
	return GetAbilitySystemComponentFromActorInfo();
}

void UNexusGameplayAbility::AddActivationOwnedTagsToSource() const
{
	if (!CostAndCooldownConfig || CostAndCooldownConfig->ActivationOwnedTags.IsEmpty())
	{
		return;
	}

	if (UAbilitySystemComponent* ASC = GetSourceAbilitySystemComponent())
	{
		ASC->AddLooseGameplayTags(CostAndCooldownConfig->ActivationOwnedTags);
	}
}

void UNexusGameplayAbility::RemoveActivationOwnedTagsFromSource() const
{
	if (!CostAndCooldownConfig || CostAndCooldownConfig->ActivationOwnedTags.IsEmpty())
	{
		return;
	}

	if (UAbilitySystemComponent* ASC = GetSourceAbilitySystemComponent())
	{
		ASC->RemoveLooseGameplayTags(CostAndCooldownConfig->ActivationOwnedTags);
	}
}

bool UNexusGameplayAbility::ApplySetByCallerEffectToOwner(
	TSubclassOf<UGameplayEffect> EffectClass,
	const FGameplayTag& DataTag,
	float Magnitude) const
{
	if (!EffectClass || !DataTag.IsValid() || !CurrentActorInfo || !CurrentActorInfo->AbilitySystemComponent.IsValid())
	{
		return false;
	}

	FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(EffectClass, GetAbilityLevel());
	if (!SpecHandle.IsValid() || !SpecHandle.Data.IsValid())
	{
		return false;
	}

	SpecHandle.Data->SetSetByCallerMagnitude(DataTag, Magnitude);

	ApplyGameplayEffectSpecToOwner(
		CurrentSpecHandle,
		CurrentActorInfo,
		CurrentActivationInfo,
		SpecHandle);

	return true;
}

void UNexusGameplayAbility::ApplyCooldownSetByCaller(float InDuration)
{
	if (!CostAndCooldownConfig || !CostAndCooldownConfig->CooldownEffectClass)
	{
		return;
	}

	const float FinalDuration = InDuration >= 0.f
		? InDuration
		: CostAndCooldownConfig->CooldownDuration;

	if (FinalDuration < 0.f)
	{
		return;
	}

	ApplySetByCallerEffectToOwner(
		CostAndCooldownConfig->CooldownEffectClass,
		CostAndCooldownConfig->CooldownSetByCallerTag,
		FinalDuration);
}

void UNexusGameplayAbility::ApplyCostSetByCaller(float InCostAmount)
{
	if (!CostAndCooldownConfig || !CostAndCooldownConfig->CostEffectClass)
	{
		return;
	}

	const float FinalCost = InCostAmount >= 0.f
		? InCostAmount
		: CostAndCooldownConfig->CostAmount * -1;
	
	
	ApplySetByCallerEffectToOwner(
		CostAndCooldownConfig->CostEffectClass,
		CostAndCooldownConfig->CostSetByCallerTag,
		FinalCost);
}

void UNexusGameplayAbility::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	RemoveActivationOwnedTagsFromSource();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}