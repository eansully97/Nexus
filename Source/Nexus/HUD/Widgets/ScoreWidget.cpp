// Fill out your copyright notice in the Description page of Project Settings.


#include "ScoreWidget.h"

#include "Nexus/GameState/NexusGameState.h"

void UScoreWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ObservedGameState = Cast<ANexusGameState>(GetWorld()->GetGameState());
}

int32 UScoreWidget::GetTeamAScore() const
{
	return ObservedGameState ? ObservedGameState->TeamAScore : 0;
}

int32 UScoreWidget::GetTeamBScore() const
{
	return ObservedGameState ? ObservedGameState->TeamBScore : 0;
}

int32 UScoreWidget::GetScoreToWin() const
{
	return ObservedGameState ? ObservedGameState->ScoreToWin : 0;
}

bool UScoreWidget::HasMatchEnded() const
{
	return ObservedGameState ? ObservedGameState->IsPostMatchActive() : false;
}

ENexusTeamID UScoreWidget::GetWinningTeam() const
{
	return ObservedGameState ? ObservedGameState->GetWinningTeam() : ENexusTeamID::Neutral;
}

int32 UScoreWidget::GetPostMatchCountdownSeconds() const
{
	return ObservedGameState ? ObservedGameState->PostMatchCountdownSeconds : 0;
}
