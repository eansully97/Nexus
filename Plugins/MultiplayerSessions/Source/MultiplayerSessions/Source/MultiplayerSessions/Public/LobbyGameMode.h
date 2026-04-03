#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "LobbyGameMode.generated.h"

UCLASS()
class MULTIPLAYERSESSIONS_API ALobbyGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	ALobbyGameMode();

	virtual void BeginPlay() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lobby")
	int32 PlayersRequiredToTravel = 2;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lobby")
	FString GameplayMapPath = TEXT("");

	UPROPERTY(BlueprintReadOnly, Category = "Lobby")
	bool bTravelStarted = false;

	void TryStartGameTravel();
};