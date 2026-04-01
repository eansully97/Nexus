// Fill out your copyright notice in the Description page of Project Settings.


#include "ScoreWidget.h"

#include "Nexus/GameState/NexusGameState.h"

void UScoreWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ObservedGameState = Cast<ANexusGameState>(GetWorld()->GetGameState());
}
