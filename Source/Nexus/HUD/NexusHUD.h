// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "NexusHUD.generated.h"

class UNexusMainHUDWidget;
/**
 * 
 */
UCLASS()
class NEXUS_API ANexusHUD : public AHUD
{
	GENERATED_BODY()
public:
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "HUD")
	UNexusMainHUDWidget* GetMainHUDWidget() const { return MainHUDWidget; }

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD")
	TSubclassOf<UNexusMainHUDWidget> MainHUDWidgetClass;

	UPROPERTY()
	TObjectPtr<UNexusMainHUDWidget> MainHUDWidget;
};

