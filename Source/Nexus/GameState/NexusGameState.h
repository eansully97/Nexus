#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "Nexus/NexusEnumTypes.h"
#include "NexusGameState.generated.h"

class UCharacterClassInfo;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnClassSelectOpenChanged, bool, bIsOpen);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAvailableClassesChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTeamScoresChanged, int32, TeamAScore, int32, TeamBScore);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMatchEndedChanged, bool, bHasEnded, ENexusTeamID, WinningTeam);

UCLASS()
class NEXUS_API ANexusGameState : public AGameState
{
	GENERATED_BODY()

public:
	UPROPERTY(ReplicatedUsing=OnRep_TeamScores, BlueprintReadOnly, Category="Match")
	int32 TeamAScore = 0;

	UPROPERTY(ReplicatedUsing=OnRep_TeamScores, BlueprintReadOnly, Category="Match")
	int32 TeamBScore = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Match")
	int32 ScoreToWin = 10;

	UPROPERTY(ReplicatedUsing=OnRep_ClassSelectOpen, BlueprintReadOnly, Category="ClassSelection")
	bool bClassSelectOpen = false;

	UPROPERTY(ReplicatedUsing=OnRep_AvailableClasses, BlueprintReadOnly, Category="ClassSelection")
	TArray<TObjectPtr<UCharacterClassInfo>> AvailableClasses;

	UPROPERTY(Replicated, BlueprintReadOnly, Category="ClassSelection")
	int32 ReadyPlayerCount = 0;

	UPROPERTY(ReplicatedUsing=OnRep_MatchEndedState, BlueprintReadOnly, Category="Match")
	bool bMatchEnded = false;

	UPROPERTY(ReplicatedUsing=OnRep_MatchEndedState, BlueprintReadOnly, Category="Match")
	ENexusTeamID WinningTeam = ENexusTeamID::Neutral;

	UPROPERTY(Replicated, BlueprintReadOnly, Category="Match")
	int32 PostMatchCountdownSeconds = 0;

	UPROPERTY(BlueprintAssignable, Category="ClassSelection")
	FOnClassSelectOpenChanged OnClassSelectOpenChanged;

	UPROPERTY(BlueprintAssignable, Category="ClassSelection")
	FOnAvailableClassesChanged OnAvailableClassesChanged;

	UPROPERTY(BlueprintAssignable, Category="Match")
	FOnTeamScoresChanged OnTeamScoresChanged;

	UPROPERTY(BlueprintAssignable, Category="Match")
	FOnMatchEndedChanged OnMatchEndedChanged;

	UFUNCTION()
	void OnRep_ClassSelectOpen();

	UFUNCTION()
	void OnRep_AvailableClasses();

	UFUNCTION()
	void OnRep_TeamScores();

	UFUNCTION()
	void OnRep_MatchEndedState();

	UFUNCTION(BlueprintPure, Category="ClassSelection")
	const TArray<UCharacterClassInfo*>& GetAvailableClasses() const
	{
		return AvailableClasses;
	}

	UFUNCTION(BlueprintPure, Category="Match")
	bool IsPostMatchActive() const
	{
		return bMatchEnded;
	}

	UFUNCTION(BlueprintPure, Category="Match")
	ENexusTeamID GetWinningTeam() const
	{
		return WinningTeam;
	}

	void SetClassSelectOpen(bool bInOpen);
	void SetAvailableClasses(const TArray<TObjectPtr<UCharacterClassInfo>>& InAvailableClasses);
	void SetTeamScores(int32 InTeamAScore, int32 InTeamBScore);
	void SetMatchEndedState(bool bInMatchEnded, ENexusTeamID InWinningTeam, int32 InPostMatchCountdownSeconds);
	void SetPostMatchCountdownSeconds(int32 InPostMatchCountdownSeconds);
	void ResetMatchFlow();

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
