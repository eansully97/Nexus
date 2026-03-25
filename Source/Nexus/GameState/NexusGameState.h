// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "NexusGameState.generated.h"

/**
 * 
 */
UCLASS()
class NEXUS_API ANexusGameState : public AGameState
{
	GENERATED_BODY()
public:
	UPROPERTY(Replicated, BlueprintReadOnly)
	int32 TeamAScore = 0;

	UPROPERTY(Replicated, BlueprintReadOnly)
	int32 TeamBScore = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int32 ScoreToWin = 10;
protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
