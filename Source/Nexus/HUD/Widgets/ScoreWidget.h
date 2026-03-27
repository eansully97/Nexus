// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ScoreWidget.generated.h"
class ANexusGameState;
class UImage;
class UTextBlock;

/**
 * 
 */
UCLASS()
class NEXUS_API UScoreWidget : public UUserWidget
{
	GENERATED_BODY()
public:

	virtual void NativeConstruct() override;
	
	UPROPERTY(BlueprintReadOnly, Category="Abilities")
	TObjectPtr<const ANexusGameState> ObservedGameState = nullptr;
};
