#include "NexusAbilitySystemComponent.h"

#include "Nexus/NexusGameplayTags.h"
#include "Nexus/Character/NexusCharacterBase.h"

UNexusAbilitySystemComponent::UNexusAbilitySystemComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UNexusAbilitySystemComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UNexusAbilitySystemComponent::AbilityInputTagPressed(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid())
	{
		return;
	}

	FGameplayTagContainer OwnedTags;
	GetOwnedGameplayTags(OwnedTags);

	// Global hard stop
	if (OwnedTags.HasTagExact(NexusGameplayTags::Status_Stunned))
	{
		return;
	}

	if (HasMatchingGameplayTag(NexusGameplayTags::Status_Ability_Active) &&
		InputTag.MatchesTag(FGameplayTag::RequestGameplayTag(TEXT("Input.Weapon"))))
	{
		return;
	}

	for (FGameplayAbilitySpec& Spec : GetActivatableAbilities())
	{
		if (Spec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
		{
			AbilitySpecInputPressed(Spec);

			if (!Spec.IsActive())
			{
				TryActivateAbility(Spec.Handle);
			}
		}
	}
}

void UNexusAbilitySystemComponent::AbilityInputTagReleased(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid())
	{
		return;
	}

	for (FGameplayAbilitySpec& Spec : GetActivatableAbilities())
	{
		if (Spec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
		{
			AbilitySpecInputReleased(Spec);
		}
	}
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