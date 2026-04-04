#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "NexusAbilityGrant.generated.h"

class UNexusGameplayAbility;

USTRUCT(BlueprintType)
struct FNexusAbilityGrant
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ability")
	TSubclassOf<UNexusGameplayAbility> Ability = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ability")
	int32 AbilityLevel = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ability", meta=(Categories="Input"))
	FGameplayTag InputTag;
};