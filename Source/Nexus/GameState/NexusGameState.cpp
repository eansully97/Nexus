#include "Nexus/GameState/NexusGameState.h"

#include "Net/UnrealNetwork.h"

void ANexusGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, TeamAScore);
	DOREPLIFETIME(ThisClass, TeamBScore);
	DOREPLIFETIME(ThisClass, bClassSelectOpen);
	DOREPLIFETIME(ThisClass, AvailableClasses);
	DOREPLIFETIME(ThisClass, ReadyPlayerCount);
	DOREPLIFETIME(ThisClass, bMatchEnded);
	DOREPLIFETIME(ThisClass, WinningTeam);
	DOREPLIFETIME(ThisClass, PostMatchCountdownSeconds);
}

void ANexusGameState::OnRep_ClassSelectOpen()
{
	OnClassSelectOpenChanged.Broadcast(bClassSelectOpen);
}

void ANexusGameState::OnRep_AvailableClasses()
{
	OnAvailableClassesChanged.Broadcast();
}

void ANexusGameState::OnRep_TeamScores()
{
	OnTeamScoresChanged.Broadcast(TeamAScore, TeamBScore);
}

void ANexusGameState::OnRep_MatchEndedState()
{
	OnMatchEndedChanged.Broadcast(bMatchEnded, WinningTeam);
}

void ANexusGameState::SetClassSelectOpen(bool bInOpen)
{
	if (bClassSelectOpen == bInOpen)
	{
		return;
	}

	bClassSelectOpen = bInOpen;
	ForceNetUpdate();
	OnRep_ClassSelectOpen();
}

void ANexusGameState::SetAvailableClasses(const TArray<TObjectPtr<UCharacterClassInfo>>& InAvailableClasses)
{
	if (AvailableClasses == InAvailableClasses)
	{
		return;
	}

	AvailableClasses = InAvailableClasses;
	ForceNetUpdate();
	OnRep_AvailableClasses();
}

void ANexusGameState::SetTeamScores(int32 InTeamAScore, int32 InTeamBScore)
{
	if (TeamAScore == InTeamAScore && TeamBScore == InTeamBScore)
	{
		return;
	}

	TeamAScore = InTeamAScore;
	TeamBScore = InTeamBScore;
	ForceNetUpdate();
	OnRep_TeamScores();
}

void ANexusGameState::SetMatchEndedState(
	bool bInMatchEnded,
	ENexusTeamID InWinningTeam,
	int32 InPostMatchCountdownSeconds)
{
	const int32 ClampedCountdownSeconds = FMath::Max(0, InPostMatchCountdownSeconds);
	const bool bDidMatchEndedChange = bMatchEnded != bInMatchEnded || WinningTeam != InWinningTeam;
	const bool bDidCountdownChange = PostMatchCountdownSeconds != ClampedCountdownSeconds;

	if (!bDidMatchEndedChange && !bDidCountdownChange)
	{
		return;
	}

	bMatchEnded = bInMatchEnded;
	WinningTeam = InWinningTeam;
	PostMatchCountdownSeconds = ClampedCountdownSeconds;
	ForceNetUpdate();

	if (bDidMatchEndedChange)
	{
		OnRep_MatchEndedState();
	}
}

void ANexusGameState::SetPostMatchCountdownSeconds(int32 InPostMatchCountdownSeconds)
{
	const int32 ClampedCountdownSeconds = FMath::Max(0, InPostMatchCountdownSeconds);
	if (PostMatchCountdownSeconds == ClampedCountdownSeconds)
	{
		return;
	}

	PostMatchCountdownSeconds = ClampedCountdownSeconds;
	ForceNetUpdate();
}

void ANexusGameState::ResetMatchFlow()
{
	const bool bDidMatchStateChange = bMatchEnded || WinningTeam != ENexusTeamID::Neutral;
	const bool bDidCountdownChange = PostMatchCountdownSeconds != 0;
	const bool bDidScoreChange = TeamAScore != 0 || TeamBScore != 0;

	bMatchEnded = false;
	WinningTeam = ENexusTeamID::Neutral;
	PostMatchCountdownSeconds = 0;
	TeamAScore = 0;
	TeamBScore = 0;
	ForceNetUpdate();

	if (bDidScoreChange)
	{
		OnRep_TeamScores();
	}

	if (bDidMatchStateChange)
	{
		OnRep_MatchEndedState();
	}
}
