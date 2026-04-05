#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Nexus/NexusAbilityGrant.h"
#include "CharacterClassInfo.generated.h"

class ANexusWeaponBase;

UENUM(BlueprintType)
enum class ECharacterClassName : uint8
{
	None,
	Warrior,
	Mage,
	Rogue
};

UCLASS()
class NEXUS_API UCharacterClassInfo : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Class")
	ECharacterClassName CharacterClassName = ECharacterClassName::None;

	// Always granted whenever this class is active.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Class|Abilities")
	TArray<FNexusAbilityGrant> DefaultClassAbilityGrants;

	// Pool the player can choose from when building a loadout.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Class|Abilities")
	TArray<FNexusAbilityGrant> SelectableClassAbilityGrants;

	// Fallback/default weapon for this class.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Class|Weapons")
	TSubclassOf<ANexusWeaponBase> DefaultWeaponClass;

	// If empty, DefaultWeaponClass is the only legal option.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Class|Weapons")
	TArray<TSubclassOf<ANexusWeaponBase>> AllowedWeaponClasses;

	UFUNCTION(BlueprintPure, Category="Class|Weapons")
	bool IsWeaponAllowed(TSubclassOf<ANexusWeaponBase> WeaponClass) const;

	UFUNCTION(BlueprintPure, Category="Class|Weapons")
	TSubclassOf<ANexusWeaponBase> GetResolvedDefaultWeaponClass() const;
};