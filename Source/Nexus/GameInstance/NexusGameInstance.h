#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "NexusGameInstance.generated.h"

class UMultiplayerSessionsSubsystem;

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

protected:
	UFUNCTION()
	void HandleCreateSessionComplete(bool bWasSuccessful);

	UFUNCTION()
	void HandleDestroySessionComplete(bool bWasSuccessful);

	UFUNCTION()
	void HandleStartSessionComplete(bool bWasSuccessful);

	void HandleFindSessionsComplete(const TArray<FOnlineSessionSearchResult>& SessionResults, bool bWasSuccessful);
	void HandleJoinSessionComplete(EOnJoinSessionCompleteResult::Type Result);

private:
	UPROPERTY()
	TObjectPtr<UMultiplayerSessionsSubsystem> MultiplayerSessionsSubsystem;

	TArray<FOnlineSessionSearchResult> CachedSearchResults;

	bool bPendingReturnToMainMenu;
	bool bQuickPlayRequested;

	void RefreshSubsystem();
	void BindSessionDelegates();
	void UnbindSessionDelegates();

	void OpenMapByName(FName MapName);
	void ServerTravelToLobby();

	FNexusSessionInfo BuildSessionInfo(const FOnlineSessionSearchResult& SearchResult, int32 SessionIndex) const;
};