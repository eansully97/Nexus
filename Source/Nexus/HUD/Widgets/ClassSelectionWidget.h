// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Nexus/DataAssets/CharacterClassInfo.h"
#include "ClassSelectionWidget.generated.h"

/**
 * 
 */
UCLASS()
class NEXUS_API UClassSelectionWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable)
	void SelectClass(UCharacterClassInfo* InClassInfo);

	UFUNCTION(BlueprintCallable)
	void ClickReady();
};
