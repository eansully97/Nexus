#include "Nexus/GameMode/NexusGameMode.h"

#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameSession.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Guid.h"
#include "Misc/PackageName.h"
#include "Nexus/Character/NexusCharacterBase.h"
#include "Nexus/Controller/NexusPlayerController.h"
#include "Nexus/Debug/NexusLog.h"
#include "Nexus/GameInstance/NexusGameInstance.h"
#include "Nexus/GameState/NexusGameState.h"
#include "Nexus/PlayerState/NexusPlayerState.h"

ANexusGameMode::ANexusGameMode()
{
	bDelayedStart = true;
}

void ANexusGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (ANexusGameState* GS = GetGameState<ANexusGameState>())
	{
		GS->ResetMatchFlow();
		GS->SetAvailableClasses(AvailableClasses);
		GS->SetClassSelectOpen(true);
	}
}

FString ANexusGameMode::InitNewPlayer(
	APlayerController* NewPlayerController,
	const FUniqueNetIdRepl& UniqueId,
	const FString& Options,
	const FString& Portal)
{
	const FString ErrorMessage = Super::InitNewPlayer(NewPlayerController, UniqueId, Options, Portal);

	CleanupExpiredReconnectReservations();

	if (NewPlayerController)
	{
		const FString ReconnectToken = UGameplayStatics::ParseOption(Options, TEXT("ReconnectToken"));
		if (!ReconnectToken.IsEmpty())
		{
			PendingReconnectTokens.Add(NewPlayerController, ReconnectToken);
		}
	}

	return ErrorMessage;
}

void ANexusGameMode::GenericPlayerInitialization(AController* C)
{
	Super::GenericPlayerInitialization(C);
	InitializeReconnectIdentity(C);
}

void ANexusGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	CleanupExpiredReconnectReservations();

	if (HasMatchEnded())
	{
		return;
	}

	if (HasPendingReconnectReservation(NewPlayer))
	{
		UE_LOG(
			LogNexus,
			Log,
			TEXT("Skipping default spawn for reconnecting player %s"),
			*GetNameSafe(NewPlayer));
		return;
	}

	Super::HandleStartingNewPlayer_Implementation(NewPlayer);
}

void ANexusGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	CleanupExpiredReconnectReservations();

	if (HasMatchEnded())
	{
		if (GameSession)
		{
			GameSession->KickPlayer(
				NewPlayer,
				FText::FromString(TEXT("This match has already ended.")));
		}

		return;
	}

	const FString ReconnectToken = ConsumePendingReconnectToken(NewPlayer);
	if (TryRestoreReconnectingPlayer(NewPlayer, ReconnectToken))
	{
		if (ANexusPlayerState* PS = NewPlayer->GetPlayerState<ANexusPlayerState>())
		{
			if (ANexusPlayerController* PC = Cast<ANexusPlayerController>(NewPlayer))
			{
				PC->ClientCacheReconnectToken(PS->GetReconnectToken());
			}
		}

		RefreshReadyStatus();

		if (ANexusPlayerController* PC = Cast<ANexusPlayerController>(NewPlayer))
		{
			PC->ClientRefreshClassSelectState();
		}

		return;
	}

	const int32 RequiredPlayerCount = GetRequiredPlayerCount();
	const bool bPreMatchClassSelectOpen = IsClassSelectionOpen();
	const bool bRosterOverflow =
		RequiredPlayerCount > 0 &&
		GetConnectedPlayerCount() > RequiredPlayerCount;

	if (!bPreMatchClassSelectOpen || bRosterOverflow)
	{
		if (GameSession)
		{
			GameSession->KickPlayer(
				NewPlayer,
				FText::FromString(TEXT("This match is already in progress or the roster is full.")));
		}

		return;
	}

	if (!AssignTeamToPlayer(NewPlayer))
	{
		if (GameSession)
		{
			GameSession->KickPlayer(
				NewPlayer,
				FText::FromString(TEXT("No team slots are available for this match.")));
		}

		return;
	}

	ApplyPlayerStateTeamToPawn(NewPlayer);

	if (ANexusPlayerState* PS = NewPlayer->GetPlayerState<ANexusPlayerState>())
	{
		PS->SetCharacterClassInfo(nullptr);
		PS->SetSelectionLockedIn(false);
	}

	RefreshReadyStatus();

	if (ANexusGameState* GS = GetGameState<ANexusGameState>())
	{
		if (GS->bClassSelectOpen)
		{
			if (ANexusPlayerController* PC = Cast<ANexusPlayerController>(NewPlayer))
			{
				PC->ClientRefreshClassSelectState();
			}
		}
	}
}

void ANexusGameMode::Logout(AController* Exiting)
{
	if (Exiting)
	{
		const TWeakObjectPtr<AController> ControllerKey(Exiting);
		if (FTimerHandle* PendingTimer = PendingRespawnTimers.Find(ControllerKey))
		{
			GetWorldTimerManager().ClearTimer(*PendingTimer);
			PendingRespawnTimers.Remove(ControllerKey);
		}

		CacheReconnectReservation(Exiting);
	}

	Super::Logout(Exiting);

	RefreshReadyStatus();
}

bool ANexusGameMode::IsClassSelectionOpen() const
{
	const ANexusGameState* GS = GetGameState<ANexusGameState>();
	return GS && GS->bClassSelectOpen && GetMatchState() == MatchState::WaitingToStart;
}

bool ANexusGameMode::IsValidClassChoice(UCharacterClassInfo* InClassInfo) const
{
	return AvailableClasses.Contains(InClassInfo);
}

void ANexusGameMode::RefreshReadyStatus()
{
	ANexusGameState* GS = GetGameState<ANexusGameState>();
	if (!GS || !GameState || HasMatchEnded())
	{
		return;
	}

	const int32 TotalPlayers = GetConnectedPlayerCount();
	const int32 ReadyPlayers = GetReadyPlayerCount();

	if (GS->ReadyPlayerCount != ReadyPlayers)
	{
		GS->ReadyPlayerCount = ReadyPlayers;
		GS->ForceNetUpdate();
	}

	if (ShouldStartMatchWithCurrentRoster(TotalPlayers, ReadyPlayers))
	{
		StartMatch();
	}
}

bool ANexusGameMode::ReadyToStartMatch_Implementation()
{
	if (HasMatchEnded())
	{
		return false;
	}

	return ShouldStartMatchWithCurrentRoster(
		GetConnectedPlayerCount(),
		GetReadyPlayerCount());
}

void ANexusGameMode::HandleMatchIsWaitingToStart()
{
	Super::HandleMatchIsWaitingToStart();

	if (ANexusGameState* GS = GetGameState<ANexusGameState>())
	{
		GS->ResetMatchFlow();
		GS->SetClassSelectOpen(true);
	}

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (ANexusPlayerController* NexusPC = Cast<ANexusPlayerController>(It->Get()))
		{
			NexusPC->ClientRefreshClassSelectState();
		}
	}
}

void ANexusGameMode::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();

	if (ANexusGameState* GS = GetGameState<ANexusGameState>())
	{
		GS->ResetMatchFlow();
		GS->SetClassSelectOpen(false);
	}

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (ANexusPlayerController* NexusPC = Cast<ANexusPlayerController>(It->Get()))
		{
			NexusPC->ClientRefreshClassSelectState();
		}
	}

	if (ClassSelectionActorClass)
	{
		TArray<AActor*> ActorsToDestroy;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ClassSelectionActorClass, ActorsToDestroy);

		for (AActor* Actor : ActorsToDestroy)
		{
			if (Actor)
			{
				Actor->Destroy();
			}
		}
	}
}

void ANexusGameMode::HandleMatchHasEnded()
{
	Super::HandleMatchHasEnded();

	ANexusGameState* GS = GetGameState<ANexusGameState>();
	if (!GS)
	{
		return;
	}

	if (!GS->HasMatchEnded())
	{
		const ENexusTeamID DerivedWinningTeam =
			GS->TeamAScore == GS->TeamBScore
				? ENexusTeamID::Neutral
				: (GS->TeamAScore > GS->TeamBScore ? ENexusTeamID::TeamA : ENexusTeamID::TeamB);

		RemainingPostMatchCountdownSeconds = FMath::Max(0, FMath::CeilToInt(PostMatchReturnDelay));
		GS->SetMatchEndedState(true, DerivedWinningTeam, RemainingPostMatchCountdownSeconds);
	}
	else
	{
		RemainingPostMatchCountdownSeconds = GS->PostMatchCountdownSeconds;
	}

	GS->SetClassSelectOpen(false);

	ClearPendingRespawnTimers();
	GetWorldTimerManager().ClearTimer(PostMatchReturnTimerHandle);
	GetWorldTimerManager().ClearTimer(PostMatchCountdownTimerHandle);
	PendingReconnectTokens.Reset();
	ReconnectReservations.Reset();

	FreezeCombatantsForPostMatch();

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (ANexusPlayerController* NexusPC = Cast<ANexusPlayerController>(It->Get()))
		{
			NexusPC->ClientRefreshClassSelectState();
		}
	}

	if (bReturnToLobbyAfterMatch && PostMatchReturnDelay <= 0.f)
	{
		TravelToLobbyAfterMatch();
		return;
	}

	if (bReturnToLobbyAfterMatch && PostMatchReturnDelay > 0.f)
	{
		GetWorldTimerManager().SetTimer(
			PostMatchReturnTimerHandle,
			this,
			&ThisClass::TravelToLobbyAfterMatch,
			PostMatchReturnDelay,
			false);

		if (RemainingPostMatchCountdownSeconds > 0)
		{
			GetWorldTimerManager().SetTimer(
				PostMatchCountdownTimerHandle,
				this,
				&ThisClass::HandlePostMatchCountdownTick,
				1.0f,
				true);
		}
	}
}

ENexusTeamID ANexusGameMode::GetNextTeamAssignment() const
{
	int32 TeamACount = 0;
	int32 TeamBCount = 0;
	const int32 MaxPlayersPerTeam = FMath::Max(1, PlayersPerTeam);

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

	if (TeamACount >= MaxPlayersPerTeam && TeamBCount >= MaxPlayersPerTeam)
	{
		return ENexusTeamID::Neutral;
	}

	if (TeamACount >= MaxPlayersPerTeam)
	{
		return ENexusTeamID::TeamB;
	}

	if (TeamBCount >= MaxPlayersPerTeam)
	{
		return ENexusTeamID::TeamA;
	}

	return (TeamACount <= TeamBCount) ? ENexusTeamID::TeamA : ENexusTeamID::TeamB;
}

bool ANexusGameMode::AssignTeamToPlayer(const AController* Controller) const
{
	if (!Controller)
	{
		return false;
	}

	ANexusPlayerState* NexusPS = Controller->GetPlayerState<ANexusPlayerState>();
	if (!NexusPS)
	{
		return false;
	}

	if (NexusPS->GetTeamID() == ENexusTeamID::Neutral)
	{
		const ENexusTeamID AssignedTeam = GetNextTeamAssignment();
		if (AssignedTeam == ENexusTeamID::Neutral)
		{
			return false;
		}

		NexusPS->SetTeamID(AssignedTeam);
	}

	return NexusPS->GetTeamID() != ENexusTeamID::Neutral;
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
		const ENexusTeamID AssignedTeam = GetNextTeamAssignment();
		if (AssignedTeam == ENexusTeamID::Neutral)
		{
			return Super::ChoosePlayerStart_Implementation(Player);
		}

		NexusPS->SetTeamID(AssignedTeam);
		UE_LOG(LogNexus, Warning, TEXT("ChoosePlayerStart had to assign team for %s -> %d"),
			*GetNameSafe(Player),
			static_cast<int32>(NexusPS->GetTeamID()));
	}

	const ENexusTeamID DesiredTeam = NexusPS->GetTeamID();

	TArray<APlayerStart*> MatchingStarts;
	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
	{
		if ((It->ActorHasTag("TeamA") && DesiredTeam == ENexusTeamID::TeamA)
			|| (It->ActorHasTag("TeamB") && DesiredTeam == ENexusTeamID::TeamB))
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
	if (!HasAuthority() || !Controller || HasMatchEnded())
	{
		return;
	}

	const TWeakObjectPtr<AController> ControllerKey(Controller);

	if (FTimerHandle* ExistingTimer = PendingRespawnTimers.Find(ControllerKey))
	{
		GetWorldTimerManager().ClearTimer(*ExistingTimer);
		PendingRespawnTimers.Remove(ControllerKey);
	}

	FTimerHandle& TimerHandle = PendingRespawnTimers.Add(ControllerKey);
	GetWorldTimerManager().SetTimer(
		TimerHandle,
		FTimerDelegate::CreateWeakLambda(
			this,
			[this, ControllerKey]()
			{
				PendingRespawnTimers.Remove(ControllerKey);

				if (!ControllerKey.IsValid())
				{
					return;
				}

				HandleRespawn(ControllerKey.Get());
			}),
		Delay,
		false);
}

void ANexusGameMode::AddScoreForTeam(ENexusTeamID TeamID, int32 Amount)
{
	ANexusGameState* GS = GetGameState<ANexusGameState>();
	if (!GS || Amount <= 0 || HasMatchEnded() || GetMatchState() != MatchState::InProgress)
	{
		return;
	}

	int32 NewTeamAScore = GS->TeamAScore;
	int32 NewTeamBScore = GS->TeamBScore;

	switch (TeamID)
	{
	case ENexusTeamID::TeamA:
		NewTeamAScore += Amount;
		break;

	case ENexusTeamID::TeamB:
		NewTeamBScore += Amount;
		break;

	default:
		break;
	}

	GS->SetTeamScores(NewTeamAScore, NewTeamBScore);

	if (NewTeamAScore >= GS->ScoreToWin || NewTeamBScore >= GS->ScoreToWin)
	{
		const ENexusTeamID WinningTeam =
			NewTeamAScore == NewTeamBScore
				? ENexusTeamID::Neutral
				: (NewTeamAScore > NewTeamBScore ? ENexusTeamID::TeamA : ENexusTeamID::TeamB);

		FinishMatchForWinner(WinningTeam);
	}
}

void ANexusGameMode::HandleRespawn(AController* Controller)
{
	if (!Controller || HasMatchEnded())
	{
		return;
	}

	PendingRespawnTimers.Remove(TWeakObjectPtr<AController>(Controller));

	APawn* OldPawn = Controller->GetPawn();
	if (OldPawn)
	{
		Controller->UnPossess();
		OldPawn->Destroy();
	}

	RestartPlayer(Controller);
}

void ANexusGameMode::FinishMatchForWinner(ENexusTeamID WinningTeam)
{
	if (HasMatchEnded())
	{
		return;
	}

	ANexusGameState* GS = GetGameState<ANexusGameState>();
	if (GS)
	{
		RemainingPostMatchCountdownSeconds = FMath::Max(0, FMath::CeilToInt(PostMatchReturnDelay));
		GS->SetMatchEndedState(true, WinningTeam, RemainingPostMatchCountdownSeconds);
	}

	EndMatch();
}

void ANexusGameMode::FreezeCombatantsForPostMatch()
{
	for (TActorIterator<ANexusCharacterBase> It(GetWorld()); It; ++It)
	{
		ANexusCharacterBase* Character = *It;
		if (!Character)
		{
			continue;
		}

		if (UCharacterMovementComponent* CharacterMovement = Character->GetCharacterMovement())
		{
			CharacterMovement->StopMovementImmediately();
			CharacterMovement->DisableMovement();
		}
	}
}

void ANexusGameMode::ClearPendingRespawnTimers()
{
	for (TPair<TWeakObjectPtr<AController>, FTimerHandle>& PendingRespawnPair : PendingRespawnTimers)
	{
		GetWorldTimerManager().ClearTimer(PendingRespawnPair.Value);
	}

	PendingRespawnTimers.Reset();
}

void ANexusGameMode::HandlePostMatchCountdownTick()
{
	ANexusGameState* GS = GetGameState<ANexusGameState>();
	if (!GS)
	{
		GetWorldTimerManager().ClearTimer(PostMatchCountdownTimerHandle);
		return;
	}

	RemainingPostMatchCountdownSeconds = FMath::Max(0, RemainingPostMatchCountdownSeconds - 1);
	GS->SetPostMatchCountdownSeconds(RemainingPostMatchCountdownSeconds);

	if (RemainingPostMatchCountdownSeconds <= 0)
	{
		GetWorldTimerManager().ClearTimer(PostMatchCountdownTimerHandle);
	}
}

FString ANexusGameMode::ResolveLobbyTravelPath() const
{
	const UNexusGameInstance* GameInstance = GetGameInstance<UNexusGameInstance>();
	if (GameInstance && !GameInstance->LobbyMapPath.IsEmpty() && FPackageName::DoesPackageExist(GameInstance->LobbyMapPath))
	{
		return GameInstance->LobbyMapPath;
	}

	static const FString PreferredLobbyPath = TEXT("/Game/Nexus/Maps/Lobby");
	if (FPackageName::DoesPackageExist(PreferredLobbyPath))
	{
		return PreferredLobbyPath;
	}

	static const FString LegacyLobbyPath = TEXT("/Game/Maps/L_Lobby");
	if (FPackageName::DoesPackageExist(LegacyLobbyPath))
	{
		return LegacyLobbyPath;
	}

	return FString();
}

void ANexusGameMode::TravelToLobbyAfterMatch()
{
	GetWorldTimerManager().ClearTimer(PostMatchReturnTimerHandle);
	GetWorldTimerManager().ClearTimer(PostMatchCountdownTimerHandle);

	if (!bReturnToLobbyAfterMatch || !GetWorld())
	{
		return;
	}

	const FString LobbyTravelPath = ResolveLobbyTravelPath();
	if (LobbyTravelPath.IsEmpty())
	{
		UE_LOG(LogNexus, Warning, TEXT("TravelToLobbyAfterMatch failed: no valid lobby map path was found"));
		return;
	}

	GetWorld()->ServerTravel(FString::Printf(TEXT("%s?listen"), *LobbyTravelPath));
}

int32 ANexusGameMode::GetConnectedPlayerCount() const
{
	if (!GameState)
	{
		return 0;
	}

	int32 ConnectedPlayers = 0;

	for (APlayerState* BasePS : GameState->PlayerArray)
	{
		if (Cast<ANexusPlayerState>(BasePS))
		{
			++ConnectedPlayers;
		}
	}

	return ConnectedPlayers;
}

int32 ANexusGameMode::GetReadyPlayerCount() const
{
	if (!GameState)
	{
		return 0;
	}

	int32 ReadyPlayers = 0;

	for (APlayerState* BasePS : GameState->PlayerArray)
	{
		const ANexusPlayerState* PS = Cast<ANexusPlayerState>(BasePS);
		if (!PS)
		{
			continue;
		}

		if (PS->GetCharacterClassInfo() && PS->GetSelectionLockedIn())
		{
			++ReadyPlayers;
		}
	}

	return ReadyPlayers;
}

int32 ANexusGameMode::GetRequiredPlayerCount() const
{
	return FMath::Max(1, PlayersPerTeam) * 2;
}

bool ANexusGameMode::ShouldStartMatchWithCurrentRoster(int32 ConnectedPlayers, int32 ReadyPlayers) const
{
	const int32 RequiredPlayerCount = GetRequiredPlayerCount();
	if (RequiredPlayerCount <= 0 || ConnectedPlayers <= 0)
	{
		return false;
	}

	if (bAllowStartWithoutFullRoster)
	{
		return ReadyPlayers == ConnectedPlayers;
	}

	return ConnectedPlayers == RequiredPlayerCount &&
		ReadyPlayers == RequiredPlayerCount;
}

void ANexusGameMode::CleanupExpiredReconnectReservations()
{
	const UWorld* World = GetWorld();
	const float CurrentWorldTimeSeconds = World ? World->GetTimeSeconds() : 0.f;

	for (auto It = ReconnectReservations.CreateIterator(); It; ++It)
	{
		if (!It.Value().IsValidAtTime(CurrentWorldTimeSeconds))
		{
			if (ANexusCharacterBase* PreservedPawn = It.Value().PreservedPawn.Get())
			{
				if (!PreservedPawn->GetController())
				{
					PreservedPawn->Destroy();
				}
			}

			It.RemoveCurrent();
		}
	}

	for (auto It = PendingReconnectTokens.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid())
		{
			It.RemoveCurrent();
		}
	}
}

FString ANexusGameMode::ConsumePendingReconnectToken(APlayerController* NewPlayer)
{
	if (!NewPlayer)
	{
		return FString();
	}

	const TWeakObjectPtr<APlayerController> ControllerKey(NewPlayer);
	const FString* PendingToken = PendingReconnectTokens.Find(ControllerKey);
	if (!PendingToken)
	{
		return FString();
	}

	const FString Token = *PendingToken;
	PendingReconnectTokens.Remove(ControllerKey);
	return Token;
}

bool ANexusGameMode::TryRestoreReconnectingPlayer(APlayerController* NewPlayer, const FString& ReconnectToken)
{
	if (!NewPlayer || ReconnectToken.IsEmpty())
	{
		return false;
	}

	CleanupExpiredReconnectReservations();

	const UWorld* World = GetWorld();
	const float CurrentWorldTimeSeconds = World ? World->GetTimeSeconds() : 0.f;
	FNexusReconnectReservation* Reservation = ReconnectReservations.Find(ReconnectToken);
	if (!Reservation || !Reservation->IsValidAtTime(CurrentWorldTimeSeconds))
	{
		ReconnectReservations.Remove(ReconnectToken);
		return false;
	}

	ANexusPlayerState* PS = NewPlayer->GetPlayerState<ANexusPlayerState>();
	if (!PS)
	{
		return false;
	}

	const TWeakObjectPtr<ANexusCharacterBase> PreservedPawn = Reservation->PreservedPawn;
	PS->ApplyReconnectSnapshot(Reservation->Snapshot);
	ReconnectReservations.Remove(ReconnectToken);

	if (ANexusCharacterBase* ExistingPawn = PreservedPawn.Get())
	{
		if (APawn* CurrentPawn = NewPlayer->GetPawn())
		{
			if (CurrentPawn != ExistingPawn)
			{
				NewPlayer->UnPossess();
				CurrentPawn->Destroy();
			}
		}

		if (AController* ExistingController = ExistingPawn->GetController())
		{
			if (ExistingController != NewPlayer)
			{
				ExistingController->UnPossess();
			}
		}

		NewPlayer->Possess(ExistingPawn);

		UE_LOG(
			LogNexus,
			Verbose,
			TEXT("Repossessed preserved pawn %s for %s"),
			*GetNameSafe(ExistingPawn),
			*GetNameSafe(NewPlayer));
	}

	if (!NewPlayer->GetPawn())
	{
		RestartPlayer(NewPlayer);

		UE_LOG(
			LogNexus,
			Warning,
			TEXT("Reconnect fallback spawned a fresh pawn for %s"),
			*GetNameSafe(NewPlayer));
	}

	ApplyPlayerStateTeamToPawn(NewPlayer);

	UE_LOG(
		LogNexus,
		Log,
		TEXT("Restored reconnect reservation for %s on team %d"),
		*GetNameSafe(NewPlayer),
		static_cast<int32>(PS->GetTeamID()));

	return true;
}

void ANexusGameMode::CacheReconnectReservation(AController* Exiting)
{
	if (!Exiting)
	{
		return;
	}

	CleanupExpiredReconnectReservations();

	ANexusPlayerState* PS = Exiting->GetPlayerState<ANexusPlayerState>();
	if (!PS || PS->GetReconnectToken().IsEmpty())
	{
		return;
	}

	ANexusCharacterBase* Character = Cast<ANexusCharacterBase>(Exiting->GetPawn());
	if (Character)
	{
		PS->CapturePersistentCombatStateFromCharacter(Character);
	}

	FNexusReconnectReservation Reservation;
	Reservation.Snapshot = PS->BuildReconnectSnapshot();
	if (ShouldPreserveDisconnectedPawn(Exiting, Character))
	{
		Reservation.PreservedPawn = Character;
	}

	Reservation.ExpireWorldTimeSeconds = (GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f) + ReconnectReservationLifetime;

	if (!Reservation.Snapshot.IsValid())
	{
		return;
	}

	if (const FNexusReconnectReservation* ExistingReservation = ReconnectReservations.Find(Reservation.Snapshot.ReconnectToken))
	{
		if (!Reservation.PreservedPawn.IsValid() && ExistingReservation->PreservedPawn.IsValid())
		{
			Reservation.PreservedPawn = ExistingReservation->PreservedPawn;
		}

		Reservation.ExpireWorldTimeSeconds = FMath::Max(
			Reservation.ExpireWorldTimeSeconds,
			ExistingReservation->ExpireWorldTimeSeconds);
	}

	UE_LOG(
		LogNexus,
		Verbose,
		TEXT("Cached reconnect reservation for %s (Token=%s, PreservedPawn=%s)"),
		*GetNameSafe(Exiting),
		*PS->GetReconnectToken(),
		Reservation.PreservedPawn.IsValid() ? *GetNameSafe(Reservation.PreservedPawn.Get()) : TEXT("None"));

	ReconnectReservations.Add(Reservation.Snapshot.ReconnectToken, MoveTemp(Reservation));
}

bool ANexusGameMode::ShouldPreserveDisconnectedPawn(const AController* Exiting, const ANexusCharacterBase* Character) const
{
	if (!bPreserveDisconnectedPawns || !Exiting || !Character)
	{
		return false;
	}

	if (GetMatchState() != MatchState::InProgress)
	{
		return false;
	}

	if (Character->GetIsDead())
	{
		return false;
	}

	return true;
}

void ANexusGameMode::InitializeReconnectIdentity(AController* Controller)
{
	ANexusPlayerState* PS = Controller ? Controller->GetPlayerState<ANexusPlayerState>() : nullptr;
	if (!PS)
	{
		return;
	}

	if (PS->GetReconnectToken().IsEmpty())
	{
		PS->SetReconnectToken(FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphens));
	}

	if (ANexusPlayerController* PC = Cast<ANexusPlayerController>(Controller))
	{
		PC->ClientCacheReconnectToken(PS->GetReconnectToken());
	}
}

bool ANexusGameMode::HasPendingReconnectReservation(APlayerController* NewPlayer) const
{
	if (!NewPlayer)
	{
		return false;
	}

	const FString* PendingReconnectToken = PendingReconnectTokens.Find(TWeakObjectPtr<APlayerController>(NewPlayer));
	if (!PendingReconnectToken || PendingReconnectToken->IsEmpty())
	{
		return false;
	}

	const FNexusReconnectReservation* Reservation = ReconnectReservations.Find(*PendingReconnectToken);
	if (!Reservation)
	{
		return false;
	}

	const float CurrentWorldTimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	return Reservation->IsValidAtTime(CurrentWorldTimeSeconds);
}
