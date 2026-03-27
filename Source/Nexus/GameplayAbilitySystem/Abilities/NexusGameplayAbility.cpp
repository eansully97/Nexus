// Fill out your copyright notice in the Description page of Project Settings.


#include "NexusGameplayAbility.h"
#include "Nexus/NexusEnumTypes.h"
#include "Nexus/DataAssets/DataAsset_AbilityInfo.h"

EAbilityContainerInfo UNexusGameplayAbility::GetContainerToShowIn() const
{
	if (!AbilityInfo)
	{
		return EAbilityContainerInfo::None;
	}
	return AbilityInfo->ContainerToShowIn;
}

FText UNexusGameplayAbility::GetAbilityDisplayName() const
{
	if (!AbilityInfo)
	{
		return FText();
	}
	return AbilityInfo->AbilityDisplayName;
}