// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayEffectClassSet.generated.h"

class UGameplayEffect;
/**
 * 
 */
UCLASS(BlueprintType)
class UNexusEffectSet : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Effects")
	TSubclassOf<UGameplayEffect> StunEffect = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Effects")
	TSubclassOf<UGameplayEffect> InstantDamageEffect = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Effects")
	TSubclassOf<UGameplayEffect> DamageWithDurationEffect = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Effects")
	TSubclassOf<UGameplayEffect> InstantHealEffect = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Effects")
	TSubclassOf<UGameplayEffect> HealWithDurationEffect = nullptr;
};