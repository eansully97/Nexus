// Fill out your copyright notice in the Description page of Project Settings.


#include "NexusGameMode.h"

#include "EngineUtils.h"
#include "Nexus/GameState/NexusGameState.h"
#include "Nexus/Character/NexusCharacterBase.h"
#include "Nexus/Debug/NexusLog.h"
#include "Nexus/PlayerStart/NexusPlayerStart.h"
#include "Nexus/PlayerState/NexusPlayerState.h"


void ANexusGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	AssignTeamToPlayer(NewPlayer);
	ApplyPlayerStateTeamToPawn(NewPlayer);
}

ENexusTeamID ANexusGameMode::GetNextTeamAssignment() const
{
	int32 TeamACount = 0;
	int32 TeamBCount = 0;

	for (APlayerState* PS : GameState->PlayerArray)
	{
		ANexusPlayerState* NexusPS = Cast<ANexusPlayerState>(PS);
		if (!NexusPS)
		{
			continue;
		}

		switch (NexusPS->GetTeamID())
		{
		case ENexusTeamID::TeamA:
			++TeamACount;
			break;

		case ENexusTeamID::TeamB:
			++TeamBCount;
			break;

		default:
			break;
		}
	}

	return (TeamACount <= TeamBCount) ? ENexusTeamID::TeamA : ENexusTeamID::TeamB;
}

void ANexusGameMode::AssignTeamToPlayer(const AController* Controller) const
{
	if (!Controller)
	{
		return;
	}

	ANexusPlayerState* NexusPS = Controller->GetPlayerState<ANexusPlayerState>();
	if (!NexusPS)
	{
		return;
	}

	if (NexusPS->GetTeamID() == ENexusTeamID::Neutral)
	{
		NexusPS->SetTeamID(GetNextTeamAssignment());
	}
}

AActor* ANexusGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	if (!Player)
	{
		return Super::ChoosePlayerStart_Implementation(Player);
	}

	ANexusPlayerState* NexusPS = Player->GetPlayerState<ANexusPlayerState>();
	if (!NexusPS)
	{
		return Super::ChoosePlayerStart_Implementation(Player);
	}

	if (NexusPS->GetTeamID() == ENexusTeamID::Neutral)
	{
		NexusPS->SetTeamID(GetNextTeamAssignment());
		UE_LOG(LogNexus, Warning, TEXT("ChoosePlayerStart had to assign team for %s -> %d"),
			*GetNameSafe(Player),
			static_cast<int32>(NexusPS->GetTeamID()));
	}

	const ENexusTeamID DesiredTeam = NexusPS->GetTeamID();

	TArray<ANexusPlayerStart*> MatchingStarts;
	for (TActorIterator<ANexusPlayerStart> It(GetWorld()); It; ++It)
	{
		if (It->TeamID == DesiredTeam)
		{
			MatchingStarts.Add(*It);
		}
	}

	if (MatchingStarts.Num() > 0)
	{
		const int32 RandomIndex = FMath::RandRange(0, MatchingStarts.Num() - 1);
		return MatchingStarts[RandomIndex];
	}

	return Super::ChoosePlayerStart_Implementation(Player);
}

void ANexusGameMode::RestartPlayer(AController* NewPlayer)
{
	Super::RestartPlayer(NewPlayer);
	ApplyPlayerStateTeamToPawn(NewPlayer);
}

void ANexusGameMode::ApplyPlayerStateTeamToPawn(const AController* Controller)
{
	if (!Controller)
	{
		return;
	}

	ANexusPlayerState* NexusPS = Controller->GetPlayerState<ANexusPlayerState>();
	if (!NexusPS)
	{
		return;
	}

	ANexusCharacterBase* NexusCharacter = Cast<ANexusCharacterBase>(Controller->GetPawn());
	if (!NexusCharacter)
	{
		return;
	}

	NexusCharacter->SetTeamID(NexusPS->GetTeamID());
}

void ANexusGameMode::RequestRespawn(AController* Controller, float Delay)
{
	if (!HasAuthority() || !Controller)
	{
		return;
	}

	FTimerHandle TimerHandle;
	FTimerDelegate TimerDel;
	TimerDel.BindUObject(this, &ANexusGameMode::HandleRespawn, Controller);

	GetWorldTimerManager().SetTimer(TimerHandle, TimerDel, Delay, false);
}

void ANexusGameMode::AddScoreForTeam(ENexusTeamID TeamID, int32 Amount)
{
	ANexusGameState* GS = GetGameState<ANexusGameState>();
	if (!GS || Amount <= 0)
	{
		return;
	}

	switch (TeamID)
	{
	case ENexusTeamID::TeamA:
		GS->TeamAScore += Amount;
		break;

	case ENexusTeamID::TeamB:
		GS->TeamBScore += Amount;
		break;

	default:
		break;
	}

	// Example win condition
	if (GS->TeamAScore >= GS->ScoreToWin || GS->TeamBScore >= GS->ScoreToWin)
	{
		// Handle match end here
	}
}

void ANexusGameMode::HandleRespawn(AController* Controller)
{
	if (!Controller)
	{
		return;
	}

	APawn* OldPawn = Controller->GetPawn();
	if (OldPawn)
	{
		Controller->UnPossess();
		OldPawn->Destroy();
	}

	RestartPlayer(Controller);
}
