#include "NexusAbilitySystemComponent.h"

#include "Nexus/Character/NexusCharacterBase.h"
#include "Nexus/Character/Player/NexusPlayerCharacter.h"
#include "Nexus/GameplayAbilitySystem/Abilities/NexusGameplayAbility.h"

namespace
{
	bool AreEquivalentAbilitySpecs(const FGameplayAbilitySpec& A, const FGameplayAbilitySpec& B)
	{
		return A.Handle == B.Handle &&
			A.Ability == B.Ability &&
			A.Level == B.Level &&
			A.SourceObject == B.SourceObject &&
			A.GetDynamicSpecSourceTags() == B.GetDynamicSpecSourceTags();
	}
}

UNexusAbilitySystemComponent::UNexusAbilitySystemComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UNexusAbilitySystemComponent::BeginPlay()
{
	Super::BeginPlay();
}

bool UNexusAbilitySystemComponent::CanProcessAbilityInput() const
{
	const ANexusPlayerCharacter* Character = Cast<ANexusPlayerCharacter>(GetOwner());
	return !Character || Character->CanAcceptGameplayInput();
}

const FGameplayAbilitySpec* UNexusAbilitySystemComponent::FindAbilitySpecByInputTag(
	const FGameplayTag& InputTag) const
{
	if (!InputTag.IsValid())
	{
		return nullptr;
	}

	for (const FGameplayAbilitySpec& Spec : GetActivatableAbilities())
	{
		if (Spec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
		{
			return &Spec;
		}
	}

	return nullptr;
}

const UNexusGameplayAbility* UNexusAbilitySystemComponent::FindNexusAbilityCDOByInputTag(
	const FGameplayTag& InputTag) const
{
	const FGameplayAbilitySpec* Spec = FindAbilitySpecByInputTag(InputTag);
	return Spec ? Cast<UNexusGameplayAbility>(Spec->Ability) : nullptr;
}

void UNexusAbilitySystemComponent::AbilityInputTagPressed(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid() || !CanProcessAbilityInput())
	{
		return;
	}

	for (FGameplayAbilitySpec& Spec : GetActivatableAbilities())
	{
		const UNexusGameplayAbility* NexusAbility = Cast<UNexusGameplayAbility>(Spec.Ability);
		if (NexusAbility && NexusAbility->bActivateByEvent)
		{
			continue;
		}

		if (!Spec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
		{
			continue;
		}

		AbilitySpecInputPressed(Spec);

		if (!Spec.IsActive())
		{
			TryActivateAbility(Spec.Handle);
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

void UNexusAbilitySystemComponent::BroadcastOwnerGrantedAbilitiesChanged() const
{
	if (ANexusCharacterBase* Character = Cast<ANexusCharacterBase>(GetOwner()))
	{
		Character->OnGrantedAbilitiesChanged.Broadcast();
		Character->OnCombatStateChanged.Broadcast();
	}
}

void UNexusAbilitySystemComponent::OnRep_ActivateAbilities()
{
	Super::OnRep_ActivateAbilities();

	bool bChanged = false;

	if (LastActivatedAbilitySpecs.Num() != ActivatableAbilities.Items.Num())
	{
		bChanged = true;
	}
	else
	{
		for (int32 Index = 0; Index < LastActivatedAbilitySpecs.Num(); ++Index)
		{
			const FGameplayAbilitySpec* OldSpec =
				LastActivatedAbilitySpecs.IsValidIndex(Index) ? &LastActivatedAbilitySpecs[Index] : nullptr;

			const FGameplayAbilitySpec* NewSpec =
				ActivatableAbilities.Items.IsValidIndex(Index) ? &ActivatableAbilities.Items[Index] : nullptr;

			if (!OldSpec || !NewSpec || !AreEquivalentAbilitySpecs(*OldSpec, *NewSpec))
			{
				bChanged = true;
				break;
			}
		}
	}

	if (bChanged)
	{
		LastActivatedAbilitySpecs = ActivatableAbilities.Items;
		BroadcastOwnerGrantedAbilitiesChanged();
	}
}
