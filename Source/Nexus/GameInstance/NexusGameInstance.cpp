#include "NexusGameInstance.h"

#include "Engine/Engine.h"
#include "Engine/NetDriver.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Kismet/GameplayStatics.h"
#include "MultiplayerSessionsSubsystem.h"
#include "Online/OnlineSessionNames.h"
#include "OnlineSubsystemNames.h"
#include "OnlineSubsystem.h"

#define LOCTEXT_NAMESPACE "NexusGameInstance"

DEFINE_LOG_CATEGORY_STATIC(LogNexusGameInstance, Log, All);

UNexusGameInstance::UNexusGameInstance()
{
	MainMenuMapName = TEXT("L_MainMenu");
	LobbyMapPath = TEXT("/Game/Maps/L_Lobby");
	DefaultGameplayMapName = TEXT("Lvl_ThirdPerson");

	DefaultPublicConnections = 6;
	DefaultMatchType = TEXT("ThreeVsThree");

	FrontendState = ENexusFrontendState::Booting;
	bSessionActionInProgress = false;
	bPendingReturnToMainMenu = false;
	bQuickPlayRequested = false;
	bReconnectRequested = false;
	bReconnectJoinInProgress = false;
	bReconnectPromptDismissed = false;
	bHasPendingReconnectTarget = false;
	bHasReconnectTarget = false;
}

void UNexusGameInstance::Init()
{
	Super::Init();

	RefreshSubsystem();
	BindSessionDelegates();

	if (GEngine)
	{
		GEngine->OnNetworkFailure().AddUObject(this, &ThisClass::HandleNetworkFailure);
		GEngine->OnTravelFailure().AddUObject(this, &ThisClass::HandleTravelFailure);
	}

	SetFrontendState(ENexusFrontendState::MainMenu);

	UE_LOG(
		LogNexusGameInstance,
		Log,
		TEXT("NexusGameInstance initialized. OnlineSubsystem=%s"),
		*GetCurrentOnlineSubsystemName().ToString());
}

void UNexusGameInstance::Shutdown()
{
	if (GEngine)
	{
		GEngine->OnNetworkFailure().RemoveAll(this);
		GEngine->OnTravelFailure().RemoveAll(this);
	}

	UnbindSessionDelegates();

	UE_LOG(LogNexusGameInstance, Log, TEXT("NexusGameInstance shutting down"));

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
	ResetReconnectState();
	SetFrontendState(ENexusFrontendState::InMatch);
	OpenMapByName(DefaultGameplayMapName);
}

void UNexusGameInstance::HostGame(int32 NumPublicConnections, const FString& MatchType)
{
	RefreshSubsystem();

	if (!MultiplayerSessionsSubsystem)
	{
		UE_LOG(LogNexusGameInstance, Warning, TEXT("HostGame failed: MultiplayerSessionsSubsystem is null"));
		OnHostCompleted.Broadcast(false);
		return;
	}

	ResetReconnectState();

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
		UE_LOG(LogNexusGameInstance, Warning, TEXT("FindGames failed: MultiplayerSessionsSubsystem is null"));
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
		UE_LOG(LogNexusGameInstance, Warning, TEXT("JoinGameByIndex failed: MultiplayerSessionsSubsystem is null"));
		OnJoinCompleted.Broadcast(false);
		return;
	}

	if (!CachedSearchResults.IsValidIndex(SessionIndex))
	{
		UE_LOG(LogNexusGameInstance, Warning, TEXT("JoinGameByIndex failed: invalid session index %d"), SessionIndex);
		OnJoinCompleted.Broadcast(false);
		return;
	}

	CachePendingReconnectTarget(CachedSearchResults[SessionIndex]);

	bSessionActionInProgress = true;
	bReconnectJoinInProgress = false;
	SetFrontendState(ENexusFrontendState::Connecting);

	MultiplayerSessionsSubsystem->JoinSession(CachedSearchResults[SessionIndex]);
}

void UNexusGameInstance::QuickPlay()
{
	ResetReconnectState();
	bQuickPlayRequested = true;
	FindGames(100);
}

void UNexusGameInstance::ReturnToMainMenu()
{
	RefreshSubsystem();

	const UWorld* World = GetWorld();
	const ENetMode NetMode = World ? World->GetNetMode() : NM_Standalone;
	const bool bCanPreserveReconnectContext =
		NetMode == NM_Client &&
		(
			CanReconnectToLastSession() ||
			(bHasPendingReconnectTarget && PendingReconnectTarget.IsValid()) ||
			IsUsingLocalTestSessions());

	if (bCanPreserveReconnectContext)
	{
		if (!CanReconnectToLastSession() && bHasPendingReconnectTarget && PendingReconnectTarget.IsValid())
		{
			PromotePendingReconnectTarget();
		}

		if (!CanReconnectToLastSession() && IsUsingLocalTestSessions())
		{
			CacheDirectReconnectTarget(TEXT("127.0.0.1:7777"));
		}

		PrepareReconnectPrompt(LOCTEXT("ReconnectPromptManualReturn", "Disconnected from the current match."));
	}
	else
	{
		ResetReconnectState();
	}

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

void UNexusGameInstance::JoinLocalTestServer(const FString& Address)
{
	const FString TravelAddress = Address.IsEmpty()
		? TEXT("127.0.0.1:7777")
		: Address;

	CacheDirectReconnectTarget(TravelAddress);
	TravelToServerAddress(TravelAddress, false);
}

FName UNexusGameInstance::GetCurrentOnlineSubsystemName() const
{
	return ResolveCurrentOnlineSubsystemName();
}

bool UNexusGameInstance::IsUsingSteamSessions() const
{
	return ResolveCurrentOnlineSubsystemName() == STEAM_SUBSYSTEM;
}

bool UNexusGameInstance::IsUsingLocalTestSessions() const
{
	return ResolveCurrentOnlineSubsystemName() == NULL_SUBSYSTEM;
}

FText UNexusGameInstance::GetOnlineModeDisplayText() const
{
	const FName SubsystemName = ResolveCurrentOnlineSubsystemName();
	if (SubsystemName == STEAM_SUBSYSTEM)
	{
		return LOCTEXT("OnlineModeSteam", "Steam Online");
	}

	if (SubsystemName == NULL_SUBSYSTEM)
	{
		return LOCTEXT("OnlineModeLocalTest", "Local Test (No Steam)");
	}

	if (SubsystemName.IsNone())
	{
		return LOCTEXT("OnlineModeUnavailable", "No Online Subsystem");
	}

	return FText::Format(
		LOCTEXT("OnlineModeOther", "Online: {0}"),
		FText::FromName(SubsystemName));
}

bool UNexusGameInstance::CanReconnectToLastSession() const
{
	return bHasReconnectTarget && LastReconnectTarget.IsValid();
}

bool UNexusGameInstance::ShouldShowReconnectPrompt() const
{
	return CanReconnectToLastSession() && !bReconnectPromptDismissed;
}

bool UNexusGameInstance::IsReconnectAttemptInProgress() const
{
	return bReconnectRequested || bReconnectJoinInProgress;
}

FNexusReconnectSessionInfo UNexusGameInstance::GetReconnectSessionInfo() const
{
	return LastReconnectTarget;
}

FText UNexusGameInstance::GetReconnectStatusText() const
{
	if (!ReconnectStatusText.IsEmpty())
	{
		return ReconnectStatusText;
	}

	if (CanReconnectToLastSession())
	{
		return FText::Format(
			LOCTEXT("ReconnectStatusDefault", "Reconnect to {0} if the session is still available."),
			BuildReconnectTargetSummary());
	}

	return LOCTEXT("ReconnectStatusUnavailable", "No reconnectable session is currently cached.");
}

void UNexusGameInstance::AttemptReconnectToLastSession()
{
	if (!CanReconnectToLastSession())
	{
		ReconnectStatusText = LOCTEXT("ReconnectAttemptNoTarget", "There is no recent session available to reconnect to.");
		BroadcastReconnectStateChanged();
		return;
	}

	if (LastReconnectTarget.bUseDirectReconnect && !LastReconnectTarget.DirectConnectAddress.IsEmpty())
	{
		bReconnectPromptDismissed = false;
		ReconnectStatusText = LOCTEXT("ReconnectDirectJoining", "Reconnecting to your previous match...");
		BroadcastReconnectStateChanged();
		TravelToServerAddress(LastReconnectTarget.DirectConnectAddress, true);
		return;
	}

	RefreshSubsystem();
	if (!MultiplayerSessionsSubsystem)
	{
		ReconnectStatusText = LOCTEXT("ReconnectAttemptNoSubsystem", "Reconnect is unavailable because the session subsystem is not ready.");
		BroadcastReconnectStateChanged();
		return;
	}

	if (IsReconnectAttemptInProgress() || bSessionActionInProgress)
	{
		return;
	}

	bQuickPlayRequested = false;
	bReconnectRequested = true;
	bReconnectJoinInProgress = false;
	bReconnectPromptDismissed = false;
	ReconnectStatusText = LOCTEXT("ReconnectSearching", "Searching for your previous match...");
	BroadcastReconnectStateChanged();

	FindGames(100);
}

void UNexusGameInstance::DismissReconnectPrompt()
{
	// Once the player declines reconnect, treat any later join as a fresh entry.
	// We cannot notify the disconnected server immediately, so its reservation will
	// still expire on its normal timeout.
	ResetReconnectState(false);
	LocalReconnectToken.Reset();
	bReconnectPromptDismissed = true;
	BroadcastReconnectStateChanged();
}

void UNexusGameInstance::CacheLocalReconnectToken(const FString& InReconnectToken)
{
	if (InReconnectToken.IsEmpty() || LocalReconnectToken == InReconnectToken)
	{
		return;
	}

	LocalReconnectToken = InReconnectToken;
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

	UE_LOG(LogNexusGameInstance, Log, TEXT("DestroySession completed. Success=%s"), bWasSuccessful ? TEXT("true") : TEXT("false"));
}

void UNexusGameInstance::HandleStartSessionComplete(bool bWasSuccessful)
{
	UE_LOG(LogNexusGameInstance, Log, TEXT("StartSession completed. Success=%s"), bWasSuccessful ? TEXT("true") : TEXT("false"));
}

void UNexusGameInstance::HandleFindSessionsComplete(
	const TArray<FOnlineSessionSearchResult>& SessionResults,
	bool bWasSuccessful)
{
	const bool bWasReconnectSearch = bReconnectRequested;

	bSessionActionInProgress = false;
	AvailableSessions.Reset();
	CachedSearchResults.Reset();

	if (!bWasSuccessful)
	{
		if (bWasReconnectSearch)
		{
			bReconnectRequested = false;
			bReconnectJoinInProgress = false;
			ReconnectStatusText = LOCTEXT("ReconnectSearchFailed", "Reconnect search failed. You can try again if the match is still online.");
			SetFrontendState(ENexusFrontendState::MainMenu);
			BroadcastReconnectStateChanged();
			return;
		}

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
	}

	OnSessionSearchResultsUpdated.Broadcast();

	if (bWasReconnectSearch)
	{
		bReconnectRequested = false;

		for (const FOnlineSessionSearchResult& SearchResult : SessionResults)
		{
			if (!DoesSearchResultMatchReconnectTarget(SearchResult))
			{
				continue;
			}

			CachePendingReconnectTarget(SearchResult);
			bReconnectJoinInProgress = true;
			bSessionActionInProgress = true;
			ReconnectStatusText = LOCTEXT("ReconnectJoining", "Rejoining your previous match...");
			SetFrontendState(ENexusFrontendState::Connecting);
			BroadcastReconnectStateChanged();
			MultiplayerSessionsSubsystem->JoinSession(SearchResult);
			return;
		}

		bReconnectJoinInProgress = false;
		ReconnectStatusText = LOCTEXT("ReconnectNotFound", "Your previous match was not found. If it is still running, try reconnecting again in a moment.");
		SetFrontendState(ENexusFrontendState::MainMenu);
		BroadcastReconnectStateChanged();
		return;
	}

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
		LogNexusGameInstance,
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

	const bool bWasReconnectJoin = bReconnectJoinInProgress;

	if (Result != EOnJoinSessionCompleteResult::Success)
	{
		if (bWasReconnectJoin)
		{
			ReconnectStatusText = LOCTEXT("ReconnectJoinFailed", "Reconnect failed while joining the session. You can try again if the match is still running.");
			SetFrontendState(ENexusFrontendState::MainMenu);
			BroadcastReconnectStateChanged();
		}
		else
		{
			SetFrontendState(ENexusFrontendState::PlayMenu);
		}

		bReconnectJoinInProgress = false;
		ClearPendingReconnectTarget();
		OnJoinCompleted.Broadcast(false);
		return;
	}

	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	if (!Subsystem)
	{
		SetFrontendState(bWasReconnectJoin ? ENexusFrontendState::MainMenu : ENexusFrontendState::PlayMenu);
		if (bWasReconnectJoin)
		{
			ReconnectStatusText = LOCTEXT("ReconnectMissingSubsystem", "Reconnect failed because the online subsystem was unavailable.");
			BroadcastReconnectStateChanged();
		}

		bReconnectJoinInProgress = false;
		ClearPendingReconnectTarget();
		OnJoinCompleted.Broadcast(false);
		return;
	}

	IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		SetFrontendState(bWasReconnectJoin ? ENexusFrontendState::MainMenu : ENexusFrontendState::PlayMenu);
		if (bWasReconnectJoin)
		{
			ReconnectStatusText = LOCTEXT("ReconnectMissingSessionInterface", "Reconnect failed because the session interface was unavailable.");
			BroadcastReconnectStateChanged();
		}

		bReconnectJoinInProgress = false;
		ClearPendingReconnectTarget();
		OnJoinCompleted.Broadcast(false);
		return;
	}

	FString Address;
	if (!SessionInterface->GetResolvedConnectString(NAME_GameSession, Address))
	{
		SetFrontendState(bWasReconnectJoin ? ENexusFrontendState::MainMenu : ENexusFrontendState::PlayMenu);
		if (bWasReconnectJoin)
		{
			ReconnectStatusText = LOCTEXT("ReconnectNoAddress", "Reconnect failed because the session address could not be resolved.");
			BroadcastReconnectStateChanged();
		}

		bReconnectJoinInProgress = false;
		ClearPendingReconnectTarget();
		OnJoinCompleted.Broadcast(false);
		return;
	}

	APlayerController* PlayerController = GetFirstLocalPlayerController();
	if (!PlayerController)
	{
		SetFrontendState(bWasReconnectJoin ? ENexusFrontendState::MainMenu : ENexusFrontendState::PlayMenu);
		if (bWasReconnectJoin)
		{
			ReconnectStatusText = LOCTEXT("ReconnectNoPlayerController", "Reconnect failed because no local player controller was available.");
			BroadcastReconnectStateChanged();
		}

		bReconnectJoinInProgress = false;
		ClearPendingReconnectTarget();
		OnJoinCompleted.Broadcast(false);
		return;
	}

	PromotePendingReconnectTarget();
	bReconnectJoinInProgress = false;
	ReconnectStatusText = LOCTEXT("ReconnectJoined", "Rejoining match...");

	SetFrontendState(ENexusFrontendState::Lobby);
	PlayerController->ClientTravel(BuildTravelURLWithReconnectToken(Address), TRAVEL_Absolute);
	OnJoinCompleted.Broadcast(true);
}

void UNexusGameInstance::HandleNetworkFailure(
	UWorld* World,
	UNetDriver* NetDriver,
	ENetworkFailure::Type FailureType,
	const FString& ErrorString)
{
	UE_LOG(
		LogNexusGameInstance,
		Warning,
		TEXT("Network failure. Type=%d Error=%s NetDriver=%s"),
		static_cast<int32>(FailureType),
		*ErrorString,
		NetDriver ? *GetNameSafe(NetDriver) : TEXT("null"));

	const bool bCanOfferReconnect =
		CanReconnectToLastSession() ||
		(bHasPendingReconnectTarget && PendingReconnectTarget.IsValid()) ||
		FrontendState == ENexusFrontendState::Lobby ||
		FrontendState == ENexusFrontendState::InMatch ||
		FrontendState == ENexusFrontendState::Connecting;

	if (!bCanOfferReconnect)
	{
		return;
	}

	if (!CanReconnectToLastSession() && bHasPendingReconnectTarget && PendingReconnectTarget.IsValid())
	{
		PromotePendingReconnectTarget();
	}

	if (!CanReconnectToLastSession() && IsUsingLocalTestSessions())
	{
		CacheDirectReconnectTarget(TEXT("127.0.0.1:7777"));
	}

	const FText ReasonText = ErrorString.IsEmpty()
		? LOCTEXT("NetworkFailureDefault", "Connection lost.")
		: FText::Format(LOCTEXT("NetworkFailureWithReason", "Connection lost: {0}"), FText::FromString(ErrorString));

	PrepareReconnectPrompt(ReasonText);
	SetFrontendState(ENexusFrontendState::MainMenu);

	if (UGameplayStatics::GetCurrentLevelName(this, true) != MainMenuMapName.ToString())
	{
		OpenMapByName(MainMenuMapName);
	}
}

void UNexusGameInstance::HandleTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ErrorString)
{
	UE_LOG(
		LogNexusGameInstance,
		Warning,
		TEXT("Travel failure. Type=%d Error=%s World=%s"),
		static_cast<int32>(FailureType),
		*ErrorString,
		World ? *GetNameSafe(World) : TEXT("null"));

	const bool bCanOfferReconnect =
		CanReconnectToLastSession() ||
		(bHasPendingReconnectTarget && PendingReconnectTarget.IsValid()) ||
		bReconnectJoinInProgress ||
		FrontendState == ENexusFrontendState::Connecting ||
		FrontendState == ENexusFrontendState::Lobby ||
		FrontendState == ENexusFrontendState::InMatch;

	if (!bCanOfferReconnect)
	{
		return;
	}

	if (!CanReconnectToLastSession() && bHasPendingReconnectTarget && PendingReconnectTarget.IsValid())
	{
		PromotePendingReconnectTarget();
	}

	if (!CanReconnectToLastSession() && IsUsingLocalTestSessions())
	{
		CacheDirectReconnectTarget(TEXT("127.0.0.1:7777"));
	}

	const FText ReasonText = ErrorString.IsEmpty()
		? LOCTEXT("TravelFailureDefault", "Travel to the match failed.")
		: FText::Format(LOCTEXT("TravelFailureWithReason", "Travel to the match failed: {0}"), FText::FromString(ErrorString));

	PrepareReconnectPrompt(ReasonText);
	SetFrontendState(ENexusFrontendState::MainMenu);

	if (UGameplayStatics::GetCurrentLevelName(this, true) != MainMenuMapName.ToString())
	{
		OpenMapByName(MainMenuMapName);
	}
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
		UE_LOG(LogNexusGameInstance, Warning, TEXT("OpenMapByName failed: MapName is None"));
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogNexusGameInstance, Warning, TEXT("OpenMapByName failed: World is null"));
		return;
	}

	UGameplayStatics::OpenLevel(World, MapName);
}

void UNexusGameInstance::ServerTravelToLobby()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogNexusGameInstance, Warning, TEXT("ServerTravelToLobby failed: World is null"));
		return;
	}

	if (LobbyMapPath.IsEmpty())
	{
		UE_LOG(LogNexusGameInstance, Warning, TEXT("ServerTravelToLobby failed: LobbyMapPath is empty"));
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

FNexusReconnectSessionInfo UNexusGameInstance::BuildReconnectSessionInfo(const FOnlineSessionSearchResult& SearchResult) const
{
	FNexusReconnectSessionInfo SessionInfo;
	SessionInfo.SessionId = SearchResult.GetSessionIdStr();
	SessionInfo.HostName = SearchResult.Session.OwningUserName;
	SessionInfo.MaxPlayers = SearchResult.Session.SessionSettings.NumPublicConnections;
	SessionInfo.bIsLAN = SearchResult.Session.SessionSettings.bIsLANMatch;
	SessionInfo.bUseDirectReconnect = false;

	FString FoundMatchType;
	if (SearchResult.Session.SessionSettings.Get(FName(TEXT("MatchType")), FoundMatchType))
	{
		SessionInfo.MatchType = FoundMatchType;
	}

	return SessionInfo;
}

bool UNexusGameInstance::DoesSearchResultMatchReconnectTarget(const FOnlineSessionSearchResult& SearchResult) const
{
	if (!CanReconnectToLastSession())
	{
		return false;
	}

	const FNexusReconnectSessionInfo Candidate = BuildReconnectSessionInfo(SearchResult);
	if (!LastReconnectTarget.SessionId.IsEmpty() &&
		LastReconnectTarget.SessionId == Candidate.SessionId)
	{
		return true;
	}

	return LastReconnectTarget.HostName == Candidate.HostName &&
		LastReconnectTarget.MatchType == Candidate.MatchType &&
		LastReconnectTarget.MaxPlayers == Candidate.MaxPlayers &&
		LastReconnectTarget.bIsLAN == Candidate.bIsLAN;
}

void UNexusGameInstance::CacheDirectReconnectTarget(const FString& Address)
{
	if (Address.IsEmpty())
	{
		return;
	}

	LastReconnectTarget.Reset();
	LastReconnectTarget.HostName = TEXT("Local Test");
	LastReconnectTarget.MatchType = TEXT("Local Test");
	LastReconnectTarget.bIsLAN = true;
	LastReconnectTarget.DirectConnectAddress = Address;
	LastReconnectTarget.bUseDirectReconnect = true;
	bHasReconnectTarget = true;
}

FString UNexusGameInstance::BuildTravelURLWithReconnectToken(const FString& BaseAddress) const
{
	if (BaseAddress.IsEmpty() || LocalReconnectToken.IsEmpty())
	{
		return BaseAddress;
	}

	if (BaseAddress.Contains(TEXT("ReconnectToken=")))
	{
		return BaseAddress;
	}

	const TCHAR JoinCharacter = BaseAddress.Contains(TEXT("?")) ? TCHAR('&') : TCHAR('?');
	return FString::Printf(TEXT("%s%cReconnectToken=%s"), *BaseAddress, JoinCharacter, *LocalReconnectToken);
}

void UNexusGameInstance::TravelToServerAddress(const FString& Address, bool bIsReconnectAttempt)
{
	APlayerController* PlayerController = GetFirstLocalPlayerController();
	if (!PlayerController)
	{
		UE_LOG(LogNexusGameInstance, Warning, TEXT("TravelToServerAddress failed: no local player controller"));
		OnJoinCompleted.Broadcast(false);
		return;
	}

	if (!bIsReconnectAttempt)
	{
		ResetReconnectState(false);
		CacheDirectReconnectTarget(Address);
	}

	bSessionActionInProgress = false;
	bReconnectRequested = false;
	bReconnectJoinInProgress = bIsReconnectAttempt;
	SetFrontendState(ENexusFrontendState::Connecting);

	const FString TravelURL = BuildTravelURLWithReconnectToken(Address);

	UE_LOG(
		LogNexusGameInstance,
		Log,
		TEXT("TravelToServerAddress: traveling to %s (Reconnect=%s)"),
		*TravelURL,
		bIsReconnectAttempt ? TEXT("true") : TEXT("false"));

	PlayerController->ClientTravel(TravelURL, TRAVEL_Absolute);
	OnJoinCompleted.Broadcast(true);
}

void UNexusGameInstance::CachePendingReconnectTarget(const FOnlineSessionSearchResult& SearchResult)
{
	PendingReconnectTarget = BuildReconnectSessionInfo(SearchResult);
	bHasPendingReconnectTarget = PendingReconnectTarget.IsValid();
}

void UNexusGameInstance::PromotePendingReconnectTarget()
{
	if (!bHasPendingReconnectTarget || !PendingReconnectTarget.IsValid())
	{
		return;
	}

	LastReconnectTarget = PendingReconnectTarget;
	bHasReconnectTarget = true;
	ClearPendingReconnectTarget();
}

void UNexusGameInstance::ClearPendingReconnectTarget()
{
	PendingReconnectTarget.Reset();
	bHasPendingReconnectTarget = false;
}

void UNexusGameInstance::ClearReconnectTarget(bool bBroadcastStateChanged)
{
	LastReconnectTarget.Reset();
	bHasReconnectTarget = false;

	if (bBroadcastStateChanged)
	{
		BroadcastReconnectStateChanged();
	}
}

void UNexusGameInstance::ResetReconnectState(bool bBroadcastStateChanged)
{
	bReconnectRequested = false;
	bReconnectJoinInProgress = false;
	bReconnectPromptDismissed = false;
	ReconnectStatusText = FText::GetEmpty();
	ClearPendingReconnectTarget();
	LastReconnectTarget.Reset();
	bHasReconnectTarget = false;

	if (bBroadcastStateChanged)
	{
		BroadcastReconnectStateChanged();
	}
}

void UNexusGameInstance::PrepareReconnectPrompt(const FText& ReasonText)
{
	bReconnectRequested = false;
	bReconnectJoinInProgress = false;
	bReconnectPromptDismissed = false;

	if (!CanReconnectToLastSession())
	{
		ReconnectStatusText = ReasonText;
		BroadcastReconnectStateChanged();
		return;
	}

	ReconnectStatusText = FText::Format(
		LOCTEXT("ReconnectPromptMessage", "{0}\nYou can reconnect to {1}."),
		ReasonText,
		BuildReconnectTargetSummary());

	BroadcastReconnectStateChanged();
}

void UNexusGameInstance::BroadcastReconnectStateChanged()
{
	OnReconnectStateChanged.Broadcast();
}

FText UNexusGameInstance::BuildReconnectTargetSummary() const
{
	if (!CanReconnectToLastSession())
	{
		return LOCTEXT("ReconnectTargetUnknown", "your previous session");
	}

	if (LastReconnectTarget.bUseDirectReconnect && !LastReconnectTarget.DirectConnectAddress.IsEmpty())
	{
		if (!LastReconnectTarget.HostName.IsEmpty())
		{
			return FText::FromString(LastReconnectTarget.HostName);
		}

		return FText::Format(
			LOCTEXT("ReconnectTargetDirectAddress", "the server at {0}"),
			FText::FromString(LastReconnectTarget.DirectConnectAddress));
	}

	if (!LastReconnectTarget.HostName.IsEmpty() && !LastReconnectTarget.MatchType.IsEmpty())
	{
		return FText::Format(
			LOCTEXT("ReconnectTargetWithHostAndMode", "{0}'s {1} match"),
			FText::FromString(LastReconnectTarget.HostName),
			FText::FromString(LastReconnectTarget.MatchType));
	}

	if (!LastReconnectTarget.MatchType.IsEmpty())
	{
		return FText::Format(
			LOCTEXT("ReconnectTargetWithMode", "the previous {0} session"),
			FText::FromString(LastReconnectTarget.MatchType));
	}

	if (!LastReconnectTarget.HostName.IsEmpty())
	{
		return FText::Format(
			LOCTEXT("ReconnectTargetWithHost", "{0}'s previous session"),
			FText::FromString(LastReconnectTarget.HostName));
	}

	return LOCTEXT("ReconnectTargetFallback", "your previous session");
}

FName UNexusGameInstance::ResolveCurrentOnlineSubsystemName() const
{
	if (const IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get())
	{
		return Subsystem->GetSubsystemName();
	}

	return NAME_None;
}

#undef LOCTEXT_NAMESPACE
