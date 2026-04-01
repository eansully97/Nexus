// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "CostAndCooldownConfig.generated.h"

class UGameplayEffect;
/**
 * 
 */
UCLASS()
class NEXUS_API UCostAndCooldownConfig : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Cooldown")
	TSubclassOf<UGameplayEffect> CooldownEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Cooldown")
	FGameplayTag CooldownSetByCallerTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Cooldown")
	float CooldownDuration = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Cost")
	TSubclassOf<UGameplayEffect> CostEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Cost")
	FGameplayTag CostSetByCallerTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Cost")
	float CostAmount = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Activation")
	FGameplayTagContainer ActivationOwnedTags;
};
