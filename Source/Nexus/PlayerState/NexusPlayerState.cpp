#include "Nexus/PlayerState/NexusPlayerState.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "Net/UnrealNetwork.h"
#include "Nexus/Character/NexusCharacterBase.h"
#include "Nexus/Character/Player/NexusPlayerCharacter.h"
#include "Nexus/Components/NexusWeaponsManager.h"
#include "Nexus/DataAssets/CostAndCooldownConfig.h"
#include "Nexus/GameplayAbilitySystem/Abilities/NexusGameplayAbility.h"
#include "Nexus/Weapons/NexusWeaponBase.h"

namespace
{
	bool ApplySetByCallerEffectToASC(
		UAbilitySystemComponent* ASC,
		TSubclassOf<UGameplayEffect> EffectClass,
		const FGameplayTag& DataTag,
		float Magnitude,
		UObject* SourceObject = nullptr,
		float EffectLevel = 1.f)
	{
		if (!ASC || !EffectClass || !DataTag.IsValid() || !FMath::IsFinite(Magnitude))
		{
			return false;
		}

		FGameplayEffectContextHandle ContextHandle = ASC->MakeEffectContext();
		if (SourceObject)
		{
			ContextHandle.AddSourceObject(SourceObject);
		}

		FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(EffectClass, EffectLevel, ContextHandle);
		if (!SpecHandle.IsValid() || !SpecHandle.Data.IsValid())
		{
			return false;
		}

		SpecHandle.Data->SetSetByCallerMagnitude(DataTag, Magnitude);
		ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
		return true;
	}

	bool AreEquivalentAbilityGrants(const FNexusAbilityGrant& A, const FNexusAbilityGrant& B)
	{
		return A.Ability == B.Ability
			&& A.AbilityLevel == B.AbilityLevel
			&& A.InputTag == B.InputTag;
	}

	bool ContainsEquivalentGrant(
		const TArray<FNexusAbilityGrant>& Grants,
		const FNexusAbilityGrant& Candidate)
	{
		for (const FNexusAbilityGrant& Existing : Grants)
		{
			if (AreEquivalentAbilityGrants(Existing, Candidate))
			{
				return true;
			}
		}

		return false;
	}
}

ANexusPlayerState::ANexusPlayerState()
{
}

void ANexusPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, TeamID);
	DOREPLIFETIME(ThisClass, CurrentLoadout);
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
	if (CurrentLoadout.SelectedClass == InInfo)
	{
		return;
	}

	CurrentLoadout.SelectedClass = InInfo;
	NormalizeCurrentLoadout();

	if (HasAuthority())
	{
		HandleProfileChanged();
	}
}

void ANexusPlayerState::SetSelectedWeaponClass(TSubclassOf<ANexusWeaponBase> InWeaponClass)
{
	if (CurrentLoadout.SelectedWeapon == InWeaponClass)
	{
		return;
	}

	CurrentLoadout.SelectedWeapon = InWeaponClass;
	NormalizeCurrentLoadout();

	if (HasAuthority())
	{
		HandleProfileChanged();
	}
}

void ANexusPlayerState::SetSelectedClassAbilityGrants(const TArray<FNexusAbilityGrant>& InAbilityGrants)
{
	CurrentLoadout.SelectedClassAbilityGrants = InAbilityGrants;
	NormalizeCurrentLoadout();

	if (HasAuthority())
	{
		HandleProfileChanged();
	}
}

void ANexusPlayerState::SetLoadout(const FNexusLoadout& InLoadout)
{
	CurrentLoadout = InLoadout;
	NormalizeCurrentLoadout();

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

void ANexusPlayerState::OnRep_CurrentLoadout()
{
	NormalizeCurrentLoadout();
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

void ANexusPlayerState::NormalizeCurrentLoadout()
{
	if (!CurrentLoadout.SelectedClass)
	{
		CurrentLoadout.SelectedWeapon = nullptr;
		CurrentLoadout.SelectedClassAbilityGrants.Reset();
		return;
	}

	if (!CurrentLoadout.SelectedWeapon ||
		!CurrentLoadout.SelectedClass->IsWeaponAllowed(CurrentLoadout.SelectedWeapon))
	{
		CurrentLoadout.SelectedWeapon = CurrentLoadout.SelectedClass->GetResolvedDefaultWeaponClass();
	}

	TArray<FNexusAbilityGrant> FilteredSelections;

	for (const FNexusAbilityGrant& SelectedGrant : CurrentLoadout.SelectedClassAbilityGrants)
	{
		if (!SelectedGrant.Ability)
		{
			continue;
		}

		if (!ContainsEquivalentGrant(CurrentLoadout.SelectedClass->SelectableClassAbilityGrants, SelectedGrant))
		{
			continue;
		}

		if (ContainsEquivalentGrant(FilteredSelections, SelectedGrant))
		{
			continue;
		}

		FilteredSelections.Add(SelectedGrant);
	}

	CurrentLoadout.SelectedClassAbilityGrants = MoveTemp(FilteredSelections);
}

bool ANexusPlayerState::IsWeaponAllowedForCurrentClass(TSubclassOf<ANexusWeaponBase> WeaponClass) const
{
	return CurrentLoadout.SelectedClass
		&& CurrentLoadout.SelectedClass->IsWeaponAllowed(WeaponClass);
}

TArray<FNexusAbilityGrant> ANexusPlayerState::GetWeaponAbilityGrants() const
{
	if (!CurrentLoadout.SelectedWeapon)
	{
		return {};
	}

	const ANexusWeaponBase* WeaponCDO =
		CurrentLoadout.SelectedWeapon->GetDefaultObject<ANexusWeaponBase>();

	if (!WeaponCDO)
	{
		return {};
	}

	return WeaponCDO->WeaponConfig.AbilityGrants;
}

TArray<FNexusAbilityGrant> ANexusPlayerState::GetClassAbilityGrants() const
{
	TArray<FNexusAbilityGrant> Result;

	if (!CurrentLoadout.SelectedClass)
	{
		return Result;
	}

	Result.Append(CurrentLoadout.SelectedClass->DefaultClassAbilityGrants);
	Result.Append(CurrentLoadout.SelectedClassAbilityGrants);

	return Result;
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

	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	TMap<FGameplayTag, float> CooldownExpiryByTag;

	TArray<FGameplayAbilitySpecHandle> Handles;
	ASC->GetAllAbilities(Handles);

	for (const FGameplayAbilitySpecHandle& Handle : Handles)
	{
		const FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(Handle);
		const UNexusGameplayAbility* AbilityCDO = Spec ? Cast<UNexusGameplayAbility>(Spec->Ability) : nullptr;
		const UCostAndCooldownConfig* Config = AbilityCDO ? AbilityCDO->CostAndCooldownConfig : nullptr;

		if (!Config || !AbilityCDO->CostAndCooldownConfig)
		{
			continue;
		}

		FGameplayTagContainer QueryTags;
		QueryTags.AddTag(AbilityCDO->CostAndCooldownConfig->CooldownIdentityTag);

		const TArray<float> RemainingTimes =
			ASC->GetActiveEffectsTimeRemaining(FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(QueryTags));

		float LongestRemaining = 0.f;
		for (const float RemainingTime : RemainingTimes)
		{
			LongestRemaining = FMath::Max(LongestRemaining, RemainingTime);
		}

		if (LongestRemaining <= 0.f)
		{
			continue;
		}

		float& SavedExpiry = CooldownExpiryByTag.FindOrAdd(Config->CooldownIdentityTag);
		SavedExpiry = FMath::Max(SavedExpiry, Now + LongestRemaining);
	}

	for (const TPair<FGameplayTag, float>& Pair : CooldownExpiryByTag)
	{
		FNexusPersistentCooldown Entry;
		Entry.CooldownTag = Pair.Key;
		Entry.ExpireWorldTimeSeconds = Pair.Value;
		PersistentCooldowns.Add(Entry);
	}

	FGameplayTagContainer OwnedTags;
	ASC->GetOwnedGameplayTags(OwnedTags);

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

float ANexusPlayerState::GetRemainingPersistentCooldownTime(
	const FNexusPersistentCooldown& PersistentCooldown) const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return 0.f;
	}

	return FMath::Max(0.f, PersistentCooldown.ExpireWorldTimeSeconds - World->GetTimeSeconds());
}

bool ANexusPlayerState::TryApplySinglePersistentCooldownToCharacter(
	ANexusCharacterBase* Character,
	const FNexusPersistentCooldown& PersistentCooldown) const
{
	if (!Character || !PersistentCooldown.CooldownTag.IsValid())
	{
		return true;
	}

	UAbilitySystemComponent* ASC = Character->GetAbilitySystemComponent();
	if (!ASC)
	{
		return false;
	}

	const float RemainingTime = GetRemainingPersistentCooldownTime(PersistentCooldown);
	if (RemainingTime <= 0.f)
	{
		return true;
	}

	{
		FGameplayTagContainer QueryTags;
		QueryTags.AddTag(PersistentCooldown.CooldownTag);

		const TArray<float> ExistingTimes =
			ASC->GetActiveEffectsTimeRemaining(FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(QueryTags));

		for (const float ExistingTime : ExistingTimes)
		{
			if (ExistingTime > 0.f)
			{
				return true;
			}
		}
	}

	TArray<FGameplayAbilitySpecHandle> Handles;
	ASC->GetAllAbilities(Handles);

	for (const FGameplayAbilitySpecHandle& Handle : Handles)
	{
		const FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(Handle);
		const UNexusGameplayAbility* AbilityCDO = Spec ? Cast<UNexusGameplayAbility>(Spec->Ability) : nullptr;
		const UCostAndCooldownConfig* Config = AbilityCDO ? AbilityCDO->CostAndCooldownConfig : nullptr;

		if (!Config ||
			!Config->CooldownEffectClass ||
			!Config->CooldownIdentityTag.IsValid() ||
			!Config->CooldownSetByCallerTag.IsValid())
		{
			continue;
		}

		if (Config->CooldownIdentityTag != PersistentCooldown.CooldownTag)
		{
			continue;
		}

		return ApplySetByCallerEffectToASC(
			ASC,
			Config->CooldownEffectClass,
			Config->CooldownSetByCallerTag,
			RemainingTime,
			Character);
	}

	return false;
}

void ANexusPlayerState::TryApplyPersistentCooldownsToCharacter(ANexusCharacterBase* Character)
{
	if (!HasAuthority() || !Character || PersistentCooldowns.IsEmpty())
	{
		return;
	}

	TArray<FNexusPersistentCooldown> StillPending;

	for (const FNexusPersistentCooldown& PersistentCooldown : PersistentCooldowns)
	{
		if (!TryApplySinglePersistentCooldownToCharacter(Character, PersistentCooldown))
		{
			StillPending.Add(PersistentCooldown);
		}
	}

	PersistentCooldowns = MoveTemp(StillPending);
}

void ANexusPlayerState::ApplyPersistentCombatProfileToCharacter(ANexusCharacterBase* Character)
{
	if (!Character || !HasAuthority())
	{
		return;
	}

	Character->SetTeamID(TeamID);
	Character->RebuildCombatLoadout();

	if (PersistentLooseTags.Num() > 0)
	{
		UAbilitySystemBlueprintLibrary::AddGameplayTags(
			Character,
			PersistentLooseTags,
			EGameplayTagReplicationState::TagOnly);
	}

	TryApplyPersistentCooldownsToCharacter(Character);
}