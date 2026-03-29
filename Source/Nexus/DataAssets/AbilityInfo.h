// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Nexus/NexusEnumTypes.h"
#include "AbilityInfo.generated.h"

/**
 * 
 */
UCLASS()
class NEXUS_API UAbilityInfo : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="AbilityInfo")
	FText AbilityDisplayName;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="AbilityInfo")
	EAbilityContainerInfo ContainerToShowIn{EAbilityContainerInfo::None};

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="AbilityInfo")
	UTexture2D* AbilityIcon;
};
