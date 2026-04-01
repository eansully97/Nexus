// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Nexus/Weapons/NexusWeaponBase.h"
#include "CharacterClassInfo.generated.h"

class ANexusWeaponBase;
class UNexusGameplayAbility;

UENUM(BlueprintType)
enum class ECharacterClassName : uint8
{
	None,
	Warrior,
	Mage,
	Rogue
};

/**
 * 
 */
UCLASS()
class NEXUS_API UCharacterClassInfo : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AbilitySystem")
	ECharacterClassName CharacterClassName;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AbilitySystem")
	TArray<TSubclassOf<UNexusGameplayAbility>> StartingAbilities;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons")
	TSubclassOf<ANexusWeaponBase> WeaponClassToEquip;
};
