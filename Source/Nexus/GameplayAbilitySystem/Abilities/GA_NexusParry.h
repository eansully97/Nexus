// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NexusGameplayAbility.h"
#include "GA_NexusParry.generated.h"

/**
 * 
 */
UCLASS()
class NEXUS_API UGA_NexusParry : public UNexusGameplayAbility
{
	GENERATED_BODY()
public:
	UGA_NexusParry();
protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	
	void HandleSuccessfulParry(const FGameplayEventData* TriggerEventData) const;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Parry")
	float StunDuration = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Parry")
	bool bPlayParryCueOnDefender = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Parry")
	bool bPlayParryCueOnAttacker = false;
};
