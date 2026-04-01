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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Activation")
	bool bRequiresValidTarget{false};

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Activation")
	bool bActivateByEvent{false};
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Nexus|Tags")
	FGameplayTag AbilityTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Nexus|Tags")
	FGameplayTag ActivationEventTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Nexus|Tags")
	FGameplayTag InputTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Nexus|Tags")
	FGameplayTag CooldownTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Nexus|Tags")
	FGameplayTag WeaponTag;

public:
	UFUNCTION(BlueprintCallable, Category="UI")
	FText GetAbilityDisplayName() const;
	
	UFUNCTION(BlueprintCallable, Category = "AbilitySystem")
	EAbilityContainerInfo GetContainerToShowIn() const;
};
