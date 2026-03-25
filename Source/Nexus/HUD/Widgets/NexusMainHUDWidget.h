// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NexusMainHUDWidget.generated.h"

class UNexusVitalsWidget;
class ANexusCharacterBase;
/**
 * 
 */
UCLASS()
class NEXUS_API UNexusMainHUDWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "HUD")
	virtual void SetObservedPawn(APawn* NewPawn);

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UNexusVitalsWidget> VitalsWidget;

protected:
	UPROPERTY(BlueprintReadOnly, Category = "HUD")
	TObjectPtr<ANexusCharacterBase> ObservedCharacter;
};
