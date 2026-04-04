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
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	ECharacterClassName CharacterClassName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TArray<FNexusAbilityGrant> ClassAbilityGrants;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<ANexusWeaponBase> WeaponClassToEquip;
};