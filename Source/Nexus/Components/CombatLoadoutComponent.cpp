#include "Nexus/Components/CombatLoadoutComponent.h"

#include "Nexus/Character/Player/NexusPlayerCharacter.h"
#include "Nexus/Components/CharacterClassComponent.h"
#include "Nexus/Components/NexusWeaponsManager.h"
#include "Nexus/PlayerState/NexusPlayerState.h"
#include "Nexus/Weapons/NexusWeaponBase.h"

UCombatLoadoutComponent::UCombatLoadoutComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(false);
}

void UCombatLoadoutComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<ANexusPlayerCharacter>(GetOwner());
	BindToObservedPlayerState();
}

void UCombatLoadoutComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindFromObservedPlayerState();
	Super::EndPlay(EndPlayReason);
}

void UCombatLoadoutComponent::RefreshFromCurrentPlayerState()
{
	if (!OwnerCharacter)
	{
		OwnerCharacter = Cast<ANexusPlayerCharacter>(GetOwner());
	}

	BindToObservedPlayerState();
	ApplyLoadoutFromPlayerState(ObservedPlayerState, true);
}

void UCombatLoadoutComponent::BindToObservedPlayerState()
{
	if (!OwnerCharacter)
	{
		OwnerCharacter = Cast<ANexusPlayerCharacter>(GetOwner());
	}

	if (!OwnerCharacter)
	{
		return;
	}

	ANexusPlayerState* NewPlayerState = OwnerCharacter->GetPlayerState<ANexusPlayerState>();
	if (ObservedPlayerState == NewPlayerState && ObservedPlayerState)
	{
		return;
	}

	UnbindFromObservedPlayerState();
	ObservedPlayerState = NewPlayerState;

	if (ObservedPlayerState)
	{
		ObservedPlayerState->OnPlayerProfileChanged.AddDynamic(
			this,
			&ThisClass::HandleObservedPlayerProfileChanged);
	}
}

void UCombatLoadoutComponent::UnbindFromObservedPlayerState()
{
	if (ObservedPlayerState)
	{
		ObservedPlayerState->OnPlayerProfileChanged.RemoveDynamic(
			this,
			&ThisClass::HandleObservedPlayerProfileChanged);
	}

	ObservedPlayerState = nullptr;
}

void UCombatLoadoutComponent::HandleObservedPlayerProfileChanged()
{
	RefreshFromCurrentPlayerState();
}

void UCombatLoadoutComponent::ApplyLoadoutFromPlayerState(
	ANexusPlayerState* PlayerState,
	bool bApplyPersistentState)
{
	if (!OwnerCharacter)
	{
		OwnerCharacter = Cast<ANexusPlayerCharacter>(GetOwner());
	}

	if (!OwnerCharacter || !PlayerState)
	{
		return;
	}

	AppliedClassInfo = PlayerState->GetCharacterClassInfo();

	if (UCharacterClassComponent* ClassComponent = OwnerCharacter->GetClassComponent())
	{
		ClassComponent->ApplyClassInfo(AppliedClassInfo);
	}

	OwnerCharacter->SetTeamID(PlayerState->GetTeamID());

	if (!OwnerCharacter->HasAuthority())
	{
		BroadcastLoadoutChanged();
		return;
	}

	OwnerCharacter->ClearAbilitySet(ENexusAbilitySource::Class);
	OwnerCharacter->ClearAbilitySet(ENexusAbilitySource::Weapon);

	if (UNexusWeaponsManager* WeaponsManager = OwnerCharacter->GetWeaponsManager())
	{
		WeaponsManager->UnequipCurrentWeapon();
	}

	const TArray<FNexusAbilityGrant> ClassGrants = PlayerState->GetClassAbilityGrants();
	if (ClassGrants.Num() > 0)
	{
		OwnerCharacter->GrantAbilitySet(
			ENexusAbilitySource::Class,
			ClassGrants,
			OwnerCharacter);
	}

	if (UNexusWeaponsManager* WeaponsManager = OwnerCharacter->GetWeaponsManager())
	{
		const TSubclassOf<ANexusWeaponBase> SelectedWeaponClass = PlayerState->GetSelectedWeaponClass();
		if (SelectedWeaponClass)
		{
			WeaponsManager->EquipOrSwap(SelectedWeaponClass);

			if (ANexusWeaponBase* EquippedWeapon = WeaponsManager->EquippedWeapon)
			{
				const TArray<FNexusAbilityGrant> WeaponGrants = PlayerState->GetWeaponAbilityGrants();
				if (WeaponGrants.Num() > 0)
				{
					OwnerCharacter->GrantAbilitySet(
						ENexusAbilitySource::Weapon,
						WeaponGrants,
						EquippedWeapon);
				}
			}
		}
	}

	if (bApplyPersistentState)
	{
		PlayerState->ApplyPersistentCombatProfileToCharacter(OwnerCharacter);
	}

	BroadcastLoadoutChanged();
}

void UCombatLoadoutComponent::BroadcastLoadoutChanged()
{
	OnGrantedAbilitiesChanged.Broadcast();
	OnLoadoutApplied.Broadcast();

	if (OwnerCharacter)
	{
		OwnerCharacter->OnGrantedAbilitiesChanged.Broadcast();
		OwnerCharacter->OnCombatStateChanged.Broadcast();
	}
}
