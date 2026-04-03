#include "LobbyGameMode.h"

#include "GameFramework/GameStateBase.h"

ALobbyGameMode::ALobbyGameMode()
{
	bUseSeamlessTravel = true;
}

void ALobbyGameMode::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(
		LogTemp,
		Log,
		TEXT("LobbyGameMode BeginPlay | PlayersRequiredToTravel=%d | GameplayMapPath=%s"),
		PlayersRequiredToTravel,
		*GameplayMapPath
	);
}

void ALobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	const int32 NumberOfPlayers = GameState ? GameState->PlayerArray.Num() : 0;

	UE_LOG(
		LogTemp,
		Log,
		TEXT("LobbyGameMode PostLogin | Player joined: %s | PlayerCount=%d"),
		*GetNameSafe(NewPlayer),
		NumberOfPlayers
	);

	TryStartGameTravel();
}

void ALobbyGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);

	const int32 NumberOfPlayers = GameState ? GameState->PlayerArray.Num() : 0;

	UE_LOG(
		LogTemp,
		Log,
		TEXT("LobbyGameMode Logout | Player left: %s | PlayerCount=%d"),
		*GetNameSafe(Exiting),
		NumberOfPlayers
	);
}

void ALobbyGameMode::TryStartGameTravel()
{
	if (bTravelStarted)
	{
		UE_LOG(LogTemp, Log, TEXT("LobbyGameMode TryStartGameTravel skipped: travel already started"));
		return;
	}

	if (!GameState)
	{
		UE_LOG(LogTemp, Warning, TEXT("LobbyGameMode TryStartGameTravel failed: GameState is null"));
		return;
	}

	const int32 NumberOfPlayers = GameState->PlayerArray.Num();
	if (NumberOfPlayers < PlayersRequiredToTravel)
	{
		UE_LOG(
			LogTemp,
			Log,
			TEXT("LobbyGameMode waiting for players | Current=%d Required=%d"),
			NumberOfPlayers,
			PlayersRequiredToTravel
		);
		return;
	}

	if (GameplayMapPath.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("LobbyGameMode travel failed: GameplayMapPath is empty"));
		return;
	}

	bTravelStarted = true;

	const FString TravelURL = FString::Printf(TEXT("%s?listen"), *GameplayMapPath);

	UE_LOG(
		LogTemp,
		Log,
		TEXT("LobbyGameMode starting server travel to: %s"),
		*TravelURL
	);

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Warning, TEXT("LobbyGameMode travel failed: World is null"));
		bTravelStarted = false;
		return;
	}

	World->ServerTravel(TravelURL);
}