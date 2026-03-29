#include "NexusAbilitySystemComponent.h"

#include "Nexus/Character/NexusCharacterBase.h"

UNexusAbilitySystemComponent::UNexusAbilitySystemComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UNexusAbilitySystemComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UNexusAbilitySystemComponent::OnRep_ActivateAbilities()
{
	Super::OnRep_ActivateAbilities();

	ANexusCharacterBase* Character = Cast<ANexusCharacterBase>(GetOwner());
	if (!Character)
	{
		return;
	}

	bool bChanged = false;

	if (LastActivatedAbilitySpecs.Num() != ActivatableAbilities.Items.Num())
	{
		bChanged = true;
	}
	else
	{
		for (int32 i = 0; i < LastActivatedAbilitySpecs.Num(); ++i)
		{
			const FGameplayAbilitySpec* OldSpec = LastActivatedAbilitySpecs.IsValidIndex(i) ? &LastActivatedAbilitySpecs[i] : nullptr;
			const FGameplayAbilitySpec* NewSpec = ActivatableAbilities.Items.IsValidIndex(i) ? &ActivatableAbilities.Items[i] : nullptr;

			if (!OldSpec || !NewSpec)
			{
				bChanged = true;
				break;
			}

			if (OldSpec->Ability != NewSpec->Ability)
			{
				bChanged = true;
				break;
			}
		}
	}

	if (bChanged)
	{
		LastActivatedAbilitySpecs = ActivatableAbilities.Items;
		Character->OnCombatStateChanged.Broadcast();
	}
}

void UNexusAbilitySystemComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}