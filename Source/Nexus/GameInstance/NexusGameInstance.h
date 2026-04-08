#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "NexusGameInstance.generated.h"

class UMultiplayerSessionsSubsystem;
class UNetDriver;

UENUM(BlueprintType)
enum class ENexusFrontendState : uint8
{
	Booting			UMETA(DisplayName = "Booting"),
	MainMenu		UMETA(DisplayName = "Main Menu"),
	PlayMenu		UMETA(DisplayName = "Play Menu"),
	Searching		UMETA(DisplayName = "Searching"),
	Hosting			UMETA(DisplayName = "Hosting"),
	Connecting		UMETA(DisplayName = "Connecting"),
	Lobby			UMETA(DisplayName = "Lobby"),
	InMatch			UMETA(DisplayName = "In Match")
};

USTRUCT(BlueprintType)
struct FNexusSessionInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Session")
	int32 SessionIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "Session")
	FString HostName;

	UPROPERTY(BlueprintReadOnly, Category = "Session")
	FString MatchType = TEXT("Unknown");

	UPROPERTY(BlueprintReadOnly, Category = "Session")
	int32 CurrentPlayers = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Session")
	int32 MaxPlayers = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Session")
	int32 PingMs = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Session")
	bool bIsLAN = false;
};

USTRUCT(BlueprintType)
struct FNexusReconnectSessionInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Session")
	FString SessionId;

	UPROPERTY(BlueprintReadOnly, Category = "Session")
	FString HostName;

	UPROPERTY(BlueprintReadOnly, Category = "Session")
	FString MatchType = TEXT("Unknown");

	UPROPERTY(BlueprintReadOnly, Category = "Session")
	int32 MaxPlayers = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Session")
	bool bIsLAN = false;

	UPROPERTY(BlueprintReadOnly, Category = "Session")
	FString DirectConnectAddress;

	UPROPERTY(BlueprintReadOnly, Category = "Session")
	bool bUseDirectReconnect = false;

	bool IsValid() const
	{
		return !SessionId.IsEmpty() || !HostName.IsEmpty() || !DirectConnectAddress.IsEmpty();
	}

	void Reset()
	{
		SessionId.Reset();
		HostName.Reset();
		MatchType = TEXT("Unknown");
		MaxPlayers = 0;
		bIsLAN = false;
		DirectConnectAddress.Reset();
		bUseDirectReconnect = false;
	}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNexusBoolResultSignature, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FNexusSimpleEventSignature);

UCLASS()
class NEXUS_API UNexusGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	UNexusGameInstance();

	virtual void Init() override;
	virtual void Shutdown() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Maps")
	FName MainMenuMapName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Maps")
	FString LobbyMapPath;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Maps")
	FName DefaultGameplayMapName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sessions")
	int32 DefaultPublicConnections;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sessions")
	FString DefaultMatchType;

	UPROPERTY(BlueprintReadOnly, Category = "Frontend")
	ENexusFrontendState FrontendState;

	UPROPERTY(BlueprintReadOnly, Category = "Sessions")
	bool bSessionActionInProgress;

	UPROPERTY(BlueprintReadOnly, Category = "Sessions")
	TArray<FNexusSessionInfo> AvailableSessions;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FNexusBoolResultSignature OnHostCompleted;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FNexusSimpleEventSignature OnSessionSearchResultsUpdated;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FNexusBoolResultSignature OnJoinCompleted;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FNexusBoolResultSignature OnReturnToMenuCompleted;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FNexusSimpleEventSignature OnReconnectStateChanged;

	UFUNCTION(BlueprintCallable, Category = "Frontend")
	void SetFrontendState(ENexusFrontendState NewState);

	UFUNCTION(BlueprintCallable, Category = "Frontend")
	void GoToMainMenu();

	UFUNCTION(BlueprintCallable, Category = "Frontend")
	void StartOfflinePractice();

	UFUNCTION(BlueprintCallable, Category = "Sessions")
	void HostGame(int32 NumPublicConnections, const FString& MatchType);

	UFUNCTION(BlueprintCallable, Category = "Sessions")
	void FindGames(int32 MaxSearchResults = 100);

	UFUNCTION(BlueprintCallable, Category = "Sessions")
	void JoinGameByIndex(int32 SessionIndex);

	UFUNCTION(BlueprintCallable, Category = "Sessions")
	void QuickPlay();

	UFUNCTION(BlueprintCallable, Category = "Sessions")
	void ReturnToMainMenu();

	UFUNCTION(BlueprintCallable, Category = "Sessions|LocalTest")
	void JoinLocalTestServer(const FString& Address = TEXT("127.0.0.1:7777"));

	UFUNCTION(BlueprintPure, Category = "Sessions")
	FName GetCurrentOnlineSubsystemName() const;

	UFUNCTION(BlueprintPure, Category = "Sessions")
	bool IsUsingSteamSessions() const;

	UFUNCTION(BlueprintPure, Category = "Sessions")
	bool IsUsingLocalTestSessions() const;

	UFUNCTION(BlueprintPure, Category = "Sessions")
	FText GetOnlineModeDisplayText() const;

	UFUNCTION(BlueprintPure, Category = "Sessions|Reconnect")
	bool CanReconnectToLastSession() const;

	UFUNCTION(BlueprintPure, Category = "Sessions|Reconnect")
	bool ShouldShowReconnectPrompt() const;

	UFUNCTION(BlueprintPure, Category = "Sessions|Reconnect")
	bool IsReconnectAttemptInProgress() const;

	UFUNCTION(BlueprintPure, Category = "Sessions|Reconnect")
	FNexusReconnectSessionInfo GetReconnectSessionInfo() const;

	UFUNCTION(BlueprintPure, Category = "Sessions|Reconnect")
	FText GetReconnectStatusText() const;

	UFUNCTION(BlueprintCallable, Category = "Sessions|Reconnect")
	void AttemptReconnectToLastSession();

	UFUNCTION(BlueprintCallable, Category = "Sessions|Reconnect")
	void DismissReconnectPrompt();

	void CacheLocalReconnectToken(const FString& InReconnectToken);

protected:
	UFUNCTION()
	void HandleCreateSessionComplete(bool bWasSuccessful);

	UFUNCTION()
	void HandleDestroySessionComplete(bool bWasSuccessful);

	UFUNCTION()
	void HandleStartSessionComplete(bool bWasSuccessful);

	void HandleFindSessionsComplete(const TArray<FOnlineSessionSearchResult>& SessionResults, bool bWasSuccessful);
	void HandleJoinSessionComplete(EOnJoinSessionCompleteResult::Type Result);
	void HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString);
	void HandleTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ErrorString);

private:
	UPROPERTY()
	TObjectPtr<UMultiplayerSessionsSubsystem> MultiplayerSessionsSubsystem;

	TArray<FOnlineSessionSearchResult> CachedSearchResults;

	bool bPendingReturnToMainMenu;
	bool bQuickPlayRequested;
	bool bReconnectRequested;
	bool bReconnectJoinInProgress;
	bool bReconnectPromptDismissed;
	bool bHasPendingReconnectTarget;
	bool bHasReconnectTarget;

	FText ReconnectStatusText;
	FNexusReconnectSessionInfo PendingReconnectTarget;
	FNexusReconnectSessionInfo LastReconnectTarget;
	FString LocalReconnectToken;

	void RefreshSubsystem();
	void BindSessionDelegates();
	void UnbindSessionDelegates();

	void OpenMapByName(FName MapName);
	void ServerTravelToLobby();

	FNexusSessionInfo BuildSessionInfo(const FOnlineSessionSearchResult& SearchResult, int32 SessionIndex) const;
	FNexusReconnectSessionInfo BuildReconnectSessionInfo(const FOnlineSessionSearchResult& SearchResult) const;
	bool DoesSearchResultMatchReconnectTarget(const FOnlineSessionSearchResult& SearchResult) const;
	void CacheDirectReconnectTarget(const FString& Address);
	FString BuildTravelURLWithReconnectToken(const FString& BaseAddress) const;
	void TravelToServerAddress(const FString& Address, bool bIsReconnectAttempt);
	void CachePendingReconnectTarget(const FOnlineSessionSearchResult& SearchResult);
	void PromotePendingReconnectTarget();
	void ClearPendingReconnectTarget();
	void ClearReconnectTarget(bool bBroadcastStateChanged = true);
	void ResetReconnectState(bool bBroadcastStateChanged = true);
	void PrepareReconnectPrompt(const FText& ReasonText);
	void BroadcastReconnectStateChanged();
	FText BuildReconnectTargetSummary() const;
	FName ResolveCurrentOnlineSubsystemName() const;
};
