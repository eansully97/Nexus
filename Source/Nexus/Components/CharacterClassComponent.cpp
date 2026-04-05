#include "Nexus/Components/CharacterClassComponent.h"

#include "Nexus/PlayerState/NexusPlayerState.h"

UCharacterClassComponent::UCharacterClassComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UCharacterClassComponent::ApplyClassFromPlayerState(ANexusPlayerState* PS)
{
	if (!PS)
	{
		ResetAppliedClass();
		return;
	}

	ApplyClassInfo(PS->GetCharacterClassInfo());
}

void UCharacterClassComponent::ApplyClassInfo(UCharacterClassInfo* InClassInfo)
{
	if (bClassApplied && AppliedClassInfo == InClassInfo)
	{
		return;
	}

	AppliedClassInfo = InClassInfo;
	bClassApplied = AppliedClassInfo != nullptr;
}

void UCharacterClassComponent::ResetAppliedClass()
{
	AppliedClassInfo = nullptr;
	bClassApplied = false;
}