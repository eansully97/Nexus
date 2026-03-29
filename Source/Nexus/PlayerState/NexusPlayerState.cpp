#include "NexusPlayerState.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Net/UnrealNetwork.h"
#include "Nexus/Character/NexusCharacterBase.h"
#include "Nexus/GameplayAbilitySystem/Abilities/NexusGameplayAbility.h"
#include "Nexus/Weapons/NexusWeaponBase.h"

ANexusPlayerState::ANexusPlayerState()
{
}

void ANexusPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, TeamID);
	DOREPLIFETIME(ThisClass, CharacterClassInfo);
	DOREPLIFETIME(ThisClass, bSelectionLockedIn);
}

void ANexusPlayerState::SetTeamID(ENexusTeamID NewTeamID)
{
	if (TeamID == NewTeamID)
	{
		return;
	}

	TeamID = NewTeamID;

	if (HasAuthority())
	{
		HandleProfileChanged();
	}
}

void ANexusPlayerState::SetCharacterClassInfo(UCharacterClassInfo* InInfo)
{
	if (CharacterClassInfo == InInfo)
	{
		return;
	}

	CharacterClassInfo = InInfo;

	if (HasAuthority())
	{
		HandleProfileChanged();
	}
}

void ANexusPlayerState::SetSelectionLockedIn(bool bLockedIn)
{
	if (bSelectionLockedIn == bLockedIn)
	{
		return;
	}

	bSelectionLockedIn = bLockedIn;

	if (HasAuthority())
	{
		HandleProfileChanged();
	}
}

void ANexusPlayerState::OnRep_TeamID()
{
	HandleProfileChanged();
}

void ANexusPlayerState::OnRep_CharacterClassInfo()
{
	HandleProfileChanged();
}

void ANexusPlayerState::OnRep_SelectionLockedIn()
{
	HandleProfileChanged();
}

void ANexusPlayerState::HandleProfileChanged()
{
	OnPlayerProfileChanged.Broadcast();
}

TArray<TSubclassOf<UNexusGameplayAbility>> ANexusPlayerState::GetClassAbilityList() const
{
	TArray<TSubclassOf<UNexusGameplayAbility>> Result;
	
	 if (CharacterClassInfo)
	 {
	 	Result = CharacterClassInfo->StartingAbilities;
	 }
	return Result;
}

TArray<TSubclassOf<UNexusGameplayAbility>> ANexusPlayerState::GetWeaponAbilityList() const
{
	if (!CharacterClassInfo || !CharacterClassInfo->WeaponClassToEquip)
	{
		return {};
	}

	const ANexusWeaponBase* WeaponCDO =
		CharacterClassInfo->WeaponClassToEquip->GetDefaultObject<ANexusWeaponBase>();

	if (!WeaponCDO)
	{
		return {};
	}

	return WeaponCDO->WeaponConfig.AbilitiesToGrant;
}

void ANexusPlayerState::CapturePersistentCombatStateFromCharacter(ANexusCharacterBase* Character)
{
	if (!HasAuthority() || !Character)
	{
		return;
	}

	UAbilitySystemComponent* ASC = Character->GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	PersistentCooldowns.Reset();
	PersistentLooseTags.Reset();

	// --- Cooldown snapshot stub ---
	// The exact implementation depends on how your abilities expose cooldown tags.
	// A practical version is:
	// 1) iterate active gameplay effects
	// 2) find effects with cooldown tags
	// 3) store remaining time by tag

	// --- Optional loose-tag persistence stub ---
	// Example policy:
	// preserve only whitelisted tags across respawn

	FGameplayTagContainer OwnedTags;
	ASC->GetOwnedGameplayTags(OwnedTags);

	// Example whitelist pattern; replace with your real tags.
	const FGameplayTag PersistentTagRoot = FGameplayTag::RequestGameplayTag(TEXT("Status.Persistent"), false);
	if (PersistentTagRoot.IsValid())
	{
		for (const FGameplayTag& Tag : OwnedTags)
		{
			if (Tag.MatchesTag(PersistentTagRoot))
			{
				PersistentLooseTags.AddTag(Tag);
			}
		}
	}
}

void ANexusPlayerState::ApplyPersistentCombatProfileToCharacter(ANexusCharacterBase* Character)
{
	if (!Character)
	{
		return;
	}

	Character->SetTeamID(TeamID);

	if (HasAuthority())
	{
		Character->ClearAbilitySet(ENexusAbilitySource::Class);
		Character->ClearAbilitySet(ENexusAbilitySource::Weapon);

		const TArray<TSubclassOf<UNexusGameplayAbility>> ClassAbilities = GetClassAbilityList();
		const TArray<TSubclassOf<UNexusGameplayAbility>> WeaponAbilities = GetWeaponAbilityList();

		Character->GrantAbilitySet(ENexusAbilitySource::Class, ClassAbilities);
		Character->GrantAbilitySet(ENexusAbilitySource::Weapon, WeaponAbilities);

		if (PersistentLooseTags.Num() > 0)
		{
			UAbilitySystemBlueprintLibrary::AddGameplayTags(
				Character,
				PersistentLooseTags,
				EGameplayTagReplicationState::TagOnly);
		}

		// TODO: Reapply cooldown gameplay effects from PersistentCooldowns.
		// This depends on your cooldown GE strategy.
	}
}