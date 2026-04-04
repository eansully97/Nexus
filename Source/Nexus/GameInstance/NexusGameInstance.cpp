#include "NexusGameInstance.h"

#include "Kismet/GameplayStatics.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Online/OnlineSessionNames.h"
#include "MultiplayerSessionsSubsystem.h"


UNexusGameInstance::UNexusGameInstance()
{
	MainMenuMapName = TEXT("L_MainMenu");
	LobbyMapPath = TEXT("/Game/Maps/L_Lobby");
	DefaultGameplayMapName = TEXT("Lvl_ThirdPerson");

	DefaultPublicConnections = 4;
	DefaultMatchType = TEXT("TwoVsTwo");

	FrontendState = ENexusFrontendState::Booting;
	bSessionActionInProgress = false;
	bPendingReturnToMainMenu = false;
	bQuickPlayRequested = false;
}

void UNexusGameInstance::Init()
{
	Super::Init();

	RefreshSubsystem();
	BindSessionDelegates();

	SetFrontendState(ENexusFrontendState::MainMenu);

	UE_LOG(LogTemp, Log, TEXT("NexusGameInstance initialized"));
}

void UNexusGameInstance::Shutdown()
{
	UnbindSessionDelegates();

	UE_LOG(LogTemp, Log, TEXT("NexusGameInstance shutting down"));

	Super::Shutdown();
}

void UNexusGameInstance::SetFrontendState(ENexusFrontendState NewState)
{
	FrontendState = NewState;
}

void UNexusGameInstance::GoToMainMenu()
{
	SetFrontendState(ENexusFrontendState::MainMenu);
	OpenMapByName(MainMenuMapName);
}

void UNexusGameInstance::StartOfflinePractice()
{
	SetFrontendState(ENexusFrontendState::InMatch);
	OpenMapByName(DefaultGameplayMapName);
}

void UNexusGameInstance::HostGame(int32 NumPublicConnections, const FString& MatchType)
{
	RefreshSubsystem();

	if (!MultiplayerSessionsSubsystem)
	{
		UE_LOG(LogTemp, Warning, TEXT("HostGame failed: MultiplayerSessionsSubsystem is null"));
		OnHostCompleted.Broadcast(false);
		return;
	}

	const int32 FinalPublicConnections = NumPublicConnections > 0 ? NumPublicConnections : DefaultPublicConnections;
	const FString FinalMatchType = MatchType.IsEmpty() ? DefaultMatchType : MatchType;

	bSessionActionInProgress = true;
	bQuickPlayRequested = false;
	AvailableSessions.Reset();
	CachedSearchResults.Reset();

	SetFrontendState(ENexusFrontendState::Hosting);
	MultiplayerSessionsSubsystem->CreateSession(FinalPublicConnections, FinalMatchType);
}

void UNexusGameInstance::FindGames(int32 MaxSearchResults)
{
	RefreshSubsystem();

	if (!MultiplayerSessionsSubsystem)
	{
		UE_LOG(LogTemp, Warning, TEXT("FindGames failed: MultiplayerSessionsSubsystem is null"));
		AvailableSessions.Reset();
		OnSessionSearchResultsUpdated.Broadcast();
		return;
	}

	bSessionActionInProgress = true;
	AvailableSessions.Reset();
	CachedSearchResults.Reset();

	SetFrontendState(ENexusFrontendState::Searching);
	MultiplayerSessionsSubsystem->FindSessions(MaxSearchResults);
}

void UNexusGameInstance::JoinGameByIndex(int32 SessionIndex)
{
	RefreshSubsystem();

	if (!MultiplayerSessionsSubsystem)
	{
		UE_LOG(LogTemp, Warning, TEXT("JoinGameByIndex failed: MultiplayerSessionsSubsystem is null"));
		OnJoinCompleted.Broadcast(false);
		return;
	}

	if (!CachedSearchResults.IsValidIndex(SessionIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("JoinGameByIndex failed: invalid session index %d"), SessionIndex);
		OnJoinCompleted.Broadcast(false);
		return;
	}

	bSessionActionInProgress = true;
	SetFrontendState(ENexusFrontendState::Connecting);

	MultiplayerSessionsSubsystem->JoinSession(CachedSearchResults[SessionIndex]);
}

void UNexusGameInstance::QuickPlay()
{
	bQuickPlayRequested = true;
	FindGames(100);
}

void UNexusGameInstance::ReturnToMainMenu()
{
	RefreshSubsystem();

	bPendingReturnToMainMenu = true;
	bSessionActionInProgress = true;

	if (MultiplayerSessionsSubsystem)
	{
		MultiplayerSessionsSubsystem->DestroySession();
	}
	else
	{
		bSessionActionInProgress = false;
		bPendingReturnToMainMenu = false;
		GoToMainMenu();
		OnReturnToMenuCompleted.Broadcast(true);
	}
}

void UNexusGameInstance::HandleCreateSessionComplete(bool bWasSuccessful)
{
	bSessionActionInProgress = false;

	if (!bWasSuccessful)
	{
		SetFrontendState(ENexusFrontendState::PlayMenu);
		OnHostCompleted.Broadcast(false);
		return;
	}

	SetFrontendState(ENexusFrontendState::Lobby);
	ServerTravelToLobby();
	OnHostCompleted.Broadcast(true);
}

void UNexusGameInstance::HandleDestroySessionComplete(bool bWasSuccessful)
{
	bSessionActionInProgress = false;

	if (bPendingReturnToMainMenu)
	{
		bPendingReturnToMainMenu = false;
		GoToMainMenu();
		OnReturnToMenuCompleted.Broadcast(true);
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("DestroySession completed. Success=%s"), bWasSuccessful ? TEXT("true") : TEXT("false"));
}

void UNexusGameInstance::HandleStartSessionComplete(bool bWasSuccessful)
{
	UE_LOG(LogTemp, Log, TEXT("StartSession completed. Success=%s"), bWasSuccessful ? TEXT("true") : TEXT("false"));
}

void UNexusGameInstance::HandleFindSessionsComplete(
	const TArray<FOnlineSessionSearchResult>& SessionResults,
	bool bWasSuccessful)
{
	bSessionActionInProgress = false;
	AvailableSessions.Reset();
	CachedSearchResults.Reset();

	if (!bWasSuccessful)
	{
		SetFrontendState(ENexusFrontendState::PlayMenu);
		OnSessionSearchResultsUpdated.Broadcast();

		if (bQuickPlayRequested)
		{
			bQuickPlayRequested = false;
			HostGame(DefaultPublicConnections, DefaultMatchType);
		}

		return;
	}

	for (int32 Index = 0; Index < SessionResults.Num(); ++Index)
	{
		CachedSearchResults.Add(SessionResults[Index]);
		AvailableSessions.Add(BuildSessionInfo(SessionResults[Index], Index));
		UE_LOG(LogTemp, Log, TEXT("AvailableSessions populated: %d"), AvailableSessions.Num());
	}

	OnSessionSearchResultsUpdated.Broadcast();

	if (bQuickPlayRequested)
	{
		bQuickPlayRequested = false;

		if (CachedSearchResults.Num() > 0)
		{
			JoinGameByIndex(0);
			return;
		}

		HostGame(DefaultPublicConnections, DefaultMatchType);
		return;
	}

	UE_LOG(
		LogTemp,
		Log,
		TEXT("HandleFindSessionsComplete: Success=%s Results=%d"),
		bWasSuccessful ? TEXT("true") : TEXT("false"),
		SessionResults.Num()
	);

	SetFrontendState(ENexusFrontendState::PlayMenu);
}

void UNexusGameInstance::HandleJoinSessionComplete(EOnJoinSessionCompleteResult::Type Result)
{
	bSessionActionInProgress = false;

	if (Result != EOnJoinSessionCompleteResult::Success)
	{
		SetFrontendState(ENexusFrontendState::PlayMenu);
		OnJoinCompleted.Broadcast(false);
		return;
	}

	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	if (!Subsystem)
	{
		SetFrontendState(ENexusFrontendState::PlayMenu);
		OnJoinCompleted.Broadcast(false);
		return;
	}

	IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		SetFrontendState(ENexusFrontendState::PlayMenu);
		OnJoinCompleted.Broadcast(false);
		return;
	}

	FString Address;
	if (!SessionInterface->GetResolvedConnectString(NAME_GameSession, Address))
	{
		SetFrontendState(ENexusFrontendState::PlayMenu);
		OnJoinCompleted.Broadcast(false);
		return;
	}

	APlayerController* PlayerController = GetFirstLocalPlayerController();
	if (!PlayerController)
	{
		SetFrontendState(ENexusFrontendState::PlayMenu);
		OnJoinCompleted.Broadcast(false);
		return;
	}

	SetFrontendState(ENexusFrontendState::Lobby);
	PlayerController->ClientTravel(Address, TRAVEL_Absolute);
	OnJoinCompleted.Broadcast(true);
}

void UNexusGameInstance::RefreshSubsystem()
{
	MultiplayerSessionsSubsystem = GetSubsystem<UMultiplayerSessionsSubsystem>();
}

void UNexusGameInstance::BindSessionDelegates()
{
	if (!MultiplayerSessionsSubsystem)
	{
		return;
	}

	MultiplayerSessionsSubsystem->MultiplayerOnCreateSessionComplete.RemoveDynamic(this, &ThisClass::HandleCreateSessionComplete);
	MultiplayerSessionsSubsystem->MultiplayerOnDestroySessionComplete.RemoveDynamic(this, &ThisClass::HandleDestroySessionComplete);
	MultiplayerSessionsSubsystem->MultiplayerOnStartSessionComplete.RemoveDynamic(this, &ThisClass::HandleStartSessionComplete);

	MultiplayerSessionsSubsystem->MultiplayerOnFindSessionsComplete.RemoveAll(this);
	MultiplayerSessionsSubsystem->MultiplayerOnJoinSessionComplete.RemoveAll(this);

	MultiplayerSessionsSubsystem->MultiplayerOnCreateSessionComplete.AddDynamic(this, &ThisClass::HandleCreateSessionComplete);
	MultiplayerSessionsSubsystem->MultiplayerOnDestroySessionComplete.AddDynamic(this, &ThisClass::HandleDestroySessionComplete);
	MultiplayerSessionsSubsystem->MultiplayerOnStartSessionComplete.AddDynamic(this, &ThisClass::HandleStartSessionComplete);

	MultiplayerSessionsSubsystem->MultiplayerOnFindSessionsComplete.AddUObject(this, &ThisClass::HandleFindSessionsComplete);
	MultiplayerSessionsSubsystem->MultiplayerOnJoinSessionComplete.AddUObject(this, &ThisClass::HandleJoinSessionComplete);
}

void UNexusGameInstance::UnbindSessionDelegates()
{
	if (!MultiplayerSessionsSubsystem)
	{
		return;
	}

	MultiplayerSessionsSubsystem->MultiplayerOnCreateSessionComplete.RemoveDynamic(this, &ThisClass::HandleCreateSessionComplete);
	MultiplayerSessionsSubsystem->MultiplayerOnDestroySessionComplete.RemoveDynamic(this, &ThisClass::HandleDestroySessionComplete);
	MultiplayerSessionsSubsystem->MultiplayerOnStartSessionComplete.RemoveDynamic(this, &ThisClass::HandleStartSessionComplete);

	MultiplayerSessionsSubsystem->MultiplayerOnFindSessionsComplete.RemoveAll(this);
	MultiplayerSessionsSubsystem->MultiplayerOnJoinSessionComplete.RemoveAll(this);
}

void UNexusGameInstance::OpenMapByName(FName MapName)
{
	if (MapName.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("OpenMapByName failed: MapName is None"));
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Warning, TEXT("OpenMapByName failed: World is null"));
		return;
	}

	UGameplayStatics::OpenLevel(World, MapName);
}

void UNexusGameInstance::ServerTravelToLobby()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Warning, TEXT("ServerTravelToLobby failed: World is null"));
		return;
	}

	if (LobbyMapPath.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("ServerTravelToLobby failed: LobbyMapPath is empty"));
		return;
	}

	const FString TravelURL = FString::Printf(TEXT("%s?listen"), *LobbyMapPath);
	World->ServerTravel(TravelURL);
}

FNexusSessionInfo UNexusGameInstance::BuildSessionInfo(
	const FOnlineSessionSearchResult& SearchResult,
	int32 SessionIndex) const
{
	FNexusSessionInfo SessionInfo;
	SessionInfo.SessionIndex = SessionIndex;
	SessionInfo.HostName = SearchResult.Session.OwningUserName;
	SessionInfo.MaxPlayers = SearchResult.Session.SessionSettings.NumPublicConnections;
	SessionInfo.CurrentPlayers = SessionInfo.MaxPlayers - SearchResult.Session.NumOpenPublicConnections;
	SessionInfo.PingMs = SearchResult.PingInMs;
	SessionInfo.bIsLAN = SearchResult.Session.SessionSettings.bIsLANMatch;

	FString FoundMatchType;
	if (SearchResult.Session.SessionSettings.Get(FName(TEXT("MatchType")), FoundMatchType))
	{
		SessionInfo.MatchType = FoundMatchType;
	}

	return SessionInfo;
}