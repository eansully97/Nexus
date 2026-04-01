// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "Nexus/NexusEnumTypes.h"
#include "NexusGameMode.generated.h"

/**
 * 
 */
UCLASS()
class NEXUS_API ANexusGameMode : public AGameMode
{
	GENERATED_BODY()
public:
	ANexusGameMode();
	
	virtual void BeginPlay() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	
	virtual bool ReadyToStartMatch_Implementation() override;
	
	virtual void HandleMatchHasStarted() override;
	virtual void HandleMatchIsWaitingToStart() override;
	
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
	
	virtual void RestartPlayer(AController* NewPlayer) override;
	void RequestRespawn(AController* Controller, float Delay = 3.f);
	
	void AddScoreForTeam(ENexusTeamID ScoringTeam, int32 Score);

	bool IsClassSelectionOpen() const;
	bool IsValidClassChoice(class UCharacterClassInfo* InClassInfo) const;
	void RefreshReadyStatus();

protected:
	UFUNCTION()
	ENexusTeamID GetNextTeamAssignment() const;

	UFUNCTION()
	void AssignTeamToPlayer(const AController* Controller) const;

	UFUNCTION()
	static void ApplyPlayerStateTeamToPawn(const AController* Controller);
	
	UFUNCTION()
	void HandleRespawn(AController* Controller);

	UPROPERTY(EditDefaultsOnly, Category="Classes")
	TArray<TObjectPtr<UCharacterClassInfo>> AvailableClasses;
};
