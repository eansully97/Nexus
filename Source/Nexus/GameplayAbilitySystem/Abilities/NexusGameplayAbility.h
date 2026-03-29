// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "Nexus/NexusEnumTypes.h"
#include "NexusGameplayAbility.generated.h"


class UAbilityInfo;
/**
 * 
 */
UCLASS()
class NEXUS_API UNexusGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="AbilityInfo")
	UAbilityInfo* AbilityInfo;
	
	UFUNCTION(BlueprintCallable, Category="UI")
	FText GetAbilityDisplayName() const;
	
	UFUNCTION(BlueprintCallable, Category = "AbilitySystem")
	EAbilityContainerInfo GetContainerToShowIn() const;
};
