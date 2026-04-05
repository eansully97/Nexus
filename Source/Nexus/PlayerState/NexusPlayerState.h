#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "GameplayTagContainer.h"
#include "Nexus/NexusAbilityGrant.h"
#include "Nexus/NexusEnumTypes.h"
#include "Nexus/DataAssets/CharacterClassInfo.h"
#include "NexusPlayerState.generated.h"

class ANexusCharacterBase;
class ANexusWeaponBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerProfileChanged);

USTRUCT(BlueprintType)
struct FNexusPersistentCooldown
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag CooldownTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ExpireWorldTimeSeconds = 0.f;
};

USTRUCT(BlueprintType)
struct FNexusLoadout
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loadout")
	TObjectPtr<UCharacterClassInfo> SelectedClass = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loadout")
	TSubclassOf<ANexusWeaponBase> SelectedWeapon = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loadout")
	TArray<FNexusAbilityGrant> SelectedClassAbilityGrants;
};

UCLASS()
class NEXUS_API ANexusPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	ANexusPlayerState();

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(ReplicatedUsing=OnRep_TeamID, EditAnywhere, BlueprintReadOnly, Category="Profile", meta=(AllowPrivateAccess="true"))
	ENexusTeamID TeamID = ENexusTeamID::Neutral;

	UPROPERTY(ReplicatedUsing=OnRep_CurrentLoadout, EditAnywhere, BlueprintReadOnly, Category="Profile", meta=(AllowPrivateAccess="true"))
	FNexusLoadout CurrentLoadout;

	UPROPERTY(ReplicatedUsing=OnRep_SelectionLockedIn, BlueprintReadOnly, Category="Profile", meta=(AllowPrivateAccess="true"))
	bool bSelectionLockedIn = false;

	UPROPERTY()
	TArray<FNexusPersistentCooldown> PersistentCooldowns;

	UPROPERTY()
	FGameplayTagContainer PersistentLooseTags;

	UFUNCTION()
	void OnRep_TeamID();

	UFUNCTION()
	void OnRep_CurrentLoadout();

	UFUNCTION()
	void OnRep_SelectionLockedIn();

	void HandleProfileChanged();
	void NormalizeCurrentLoadout();

	float GetRemainingPersistentCooldownTime(const FNexusPersistentCooldown& PersistentCooldown) const;

	bool TryApplySinglePersistentCooldownToCharacter(
		ANexusCharacterBase* Character,
		const FNexusPersistentCooldown& PersistentCooldown) const;

public:
	UFUNCTION(BlueprintCallable, Category="Profile")
	void SetTeamID(ENexusTeamID NewTeamID);

	UFUNCTION(BlueprintPure, Category="Profile")
	ENexusTeamID GetTeamID() const { return TeamID; }

	UFUNCTION(BlueprintCallable, Category="Profile")
	void SetCharacterClassInfo(UCharacterClassInfo* InInfo);

	UFUNCTION(BlueprintPure, Category="Profile")
	UCharacterClassInfo* GetCharacterClassInfo() const { return CurrentLoadout.SelectedClass; }

	UFUNCTION(BlueprintCallable, Category="Profile")
	void SetSelectedWeaponClass(TSubclassOf<ANexusWeaponBase> InWeaponClass);

	UFUNCTION(BlueprintPure, Category="Profile")
	TSubclassOf<ANexusWeaponBase> GetSelectedWeaponClass() const { return CurrentLoadout.SelectedWeapon; }

	UFUNCTION(BlueprintCallable, Category="Profile")
	void SetSelectedClassAbilityGrants(const TArray<FNexusAbilityGrant>& InAbilityGrants);

	UFUNCTION(BlueprintPure, Category="Profile")
	const TArray<FNexusAbilityGrant>& GetSelectedClassAbilityGrants() const
	{
		return CurrentLoadout.SelectedClassAbilityGrants;
	}

	UFUNCTION(BlueprintCallable, Category="Profile")
	void SetLoadout(const FNexusLoadout& InLoadout);

	UFUNCTION(BlueprintPure, Category="Profile")
	const FNexusLoadout& GetLoadout() const { return CurrentLoadout; }

	UFUNCTION(BlueprintCallable, Category="Profile")
	void SetSelectionLockedIn(bool bLockedIn);

	UFUNCTION(BlueprintPure, Category="Profile")
	bool GetSelectionLockedIn() const { return bSelectionLockedIn; }

	UFUNCTION(BlueprintPure, Category="Profile")
	bool IsWeaponAllowedForCurrentClass(TSubclassOf<ANexusWeaponBase> WeaponClass) const;

	void CapturePersistentCombatStateFromCharacter(ANexusCharacterBase* Character);
	void ApplyPersistentCombatProfileToCharacter(ANexusCharacterBase* Character);
	void TryApplyPersistentCooldownsToCharacter(ANexusCharacterBase* Character);

	TArray<FNexusAbilityGrant> GetWeaponAbilityGrants() const;
	TArray<FNexusAbilityGrant> GetClassAbilityGrants() const;

public:
	UPROPERTY(BlueprintAssignable, Category="Events")
	FOnPlayerProfileChanged OnPlayerProfileChanged;
};