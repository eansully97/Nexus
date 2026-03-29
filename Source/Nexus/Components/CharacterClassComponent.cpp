// Fill out your copyright notice in the Description page of Project Settings.


#include "CharacterClassComponent.h"


#include "Nexus/PlayerState/NexusPlayerState.h"


UCharacterClassComponent::UCharacterClassComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UCharacterClassComponent::ApplyClassFromPlayerState(ANexusPlayerState* PS)
{
	if (!PS || !PS->GetCharacterClassInfo())
	{
		return;
	}

	if (bClassApplied && AppliedClassInfo == PS->GetCharacterClassInfo())
	{
		return;
	}

	AppliedClassInfo = PS->GetCharacterClassInfo();
	bClassApplied = true;
}

void UCharacterClassComponent::ResetAppliedClass()
{
	AppliedClassInfo = nullptr;
	bClassApplied = false;
}