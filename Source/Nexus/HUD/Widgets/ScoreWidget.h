// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Nexus/NexusEnumTypes.h"
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

	UFUNCTION(BlueprintPure, Category="Score")
	int32 GetTeamAScore() const;

	UFUNCTION(BlueprintPure, Category="Score")
	int32 GetTeamBScore() const;

	UFUNCTION(BlueprintPure, Category="Score")
	int32 GetScoreToWin() const;

	UFUNCTION(BlueprintPure, Category="Score")
	bool HasMatchEnded() const;

	UFUNCTION(BlueprintPure, Category="Score")
	ENexusTeamID GetWinningTeam() const;

	UFUNCTION(BlueprintPure, Category="Score")
	int32 GetPostMatchCountdownSeconds() const;
	
	UPROPERTY(BlueprintReadOnly, Category="Abilities")
	TObjectPtr<const ANexusGameState> ObservedGameState = nullptr;
};
