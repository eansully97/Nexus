#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "Nexus/NexusEnumTypes.h"
#include "Nexus/PlayerState/NexusPlayerState.h"
#include "NexusGameMode.generated.h"

class UCharacterClassInfo;
class ANexusCharacterBase;

USTRUCT()
struct FNexusReconnectReservation
{
	GENERATED_BODY()

	UPROPERTY()
	FNexusReconnectPlayerSnapshot Snapshot;

	UPROPERTY()
	TWeakObjectPtr<ANexusCharacterBase> PreservedPawn;

	UPROPERTY()
	float ExpireWorldTimeSeconds = 0.f;

	bool IsValidAtTime(const float CurrentWorldTimeSeconds) const
	{
		return Snapshot.IsValid() && ExpireWorldTimeSeconds > CurrentWorldTimeSeconds;
	}
};

UCLASS()
class NEXUS_API ANexusGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	ANexusGameMode();

	virtual void BeginPlay() override;
	virtual FString InitNewPlayer(
		APlayerController* NewPlayerController,
		const FUniqueNetIdRepl& UniqueId,
		const FString& Options,
		const FString& Portal) override;
	virtual void GenericPlayerInitialization(AController* C) override;
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;

	virtual bool ReadyToStartMatch_Implementation() override;

	virtual void HandleMatchHasEnded() override;
	virtual void HandleMatchHasStarted() override;
	virtual void HandleMatchIsWaitingToStart() override;

	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

	virtual void RestartPlayer(AController* NewPlayer) override;
	void RequestRespawn(AController* Controller, float Delay = 3.f);

	void AddScoreForTeam(ENexusTeamID ScoringTeam, int32 Score);

	bool IsClassSelectionOpen() const;
	bool IsValidClassChoice(UCharacterClassInfo* InClassInfo) const;
	void RefreshReadyStatus();
	bool ShouldPreserveDisconnectedPawn(const AController* Exiting, const ANexusCharacterBase* Character) const;
	void CacheReconnectReservation(AController* Exiting);

protected:
	UFUNCTION()
	ENexusTeamID GetNextTeamAssignment() const;

	UFUNCTION()
	bool AssignTeamToPlayer(const AController* Controller) const;

	UFUNCTION()
	static void ApplyPlayerStateTeamToPawn(const AController* Controller);

	UFUNCTION()
	void HandleRespawn(AController* Controller);

	UPROPERTY(EditDefaultsOnly, Category="Classes")
	TArray<TObjectPtr<UCharacterClassInfo>> AvailableClasses;

	UPROPERTY(EditDefaultsOnly, Category="ClassSelection")
	TSubclassOf<AActor> ClassSelectionActorClass;

	UPROPERTY(EditDefaultsOnly, Category="Match")
	int32 PlayersPerTeam = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Match|Testing")
	bool bAllowStartWithoutFullRoster = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Match|Reconnect", meta=(AllowPrivateAccess="true", ClampMin="10.0"))
	float ReconnectReservationLifetime = 180.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Match|Reconnect")
	bool bPreserveDisconnectedPawns = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Match|End", meta=(ClampMin="0.0"))
	float PostMatchReturnDelay = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Match|End")
	bool bReturnToLobbyAfterMatch = true;

	TMap<TWeakObjectPtr<AController>, FTimerHandle> PendingRespawnTimers;
	TMap<TWeakObjectPtr<APlayerController>, FString> PendingReconnectTokens;
	TMap<FString, FNexusReconnectReservation> ReconnectReservations;

	FTimerHandle PostMatchReturnTimerHandle;
	FTimerHandle PostMatchCountdownTimerHandle;
	int32 RemainingPostMatchCountdownSeconds = 0;

	int32 GetConnectedPlayerCount() const;
	int32 GetReadyPlayerCount() const;
	int32 GetRequiredPlayerCount() const;
	bool ShouldStartMatchWithCurrentRoster(int32 ConnectedPlayers, int32 ReadyPlayers) const;
	void CleanupExpiredReconnectReservations();
	FString ConsumePendingReconnectToken(APlayerController* NewPlayer);
	bool TryRestoreReconnectingPlayer(APlayerController* NewPlayer, const FString& ReconnectToken);
	void InitializeReconnectIdentity(AController* Controller);
	bool HasPendingReconnectReservation(APlayerController* NewPlayer) const;
	void FinishMatchForWinner(ENexusTeamID WinningTeam);
	void FreezeCombatantsForPostMatch();
	void ClearPendingRespawnTimers();
	void HandlePostMatchCountdownTick();
	void TravelToLobbyAfterMatch();
	FString ResolveLobbyTravelPath() const;
};
