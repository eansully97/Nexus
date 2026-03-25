// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "Nexus/NexusTeamTypes.h"
#include "NexusPlayerState.generated.h"

/**
 * 
 */
UCLASS()
class NEXUS_API ANexusPlayerState : public APlayerState
{
	GENERATED_BODY()
public:
	ANexusPlayerState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable)
	void SetTeamID(ENexusTeamID NewTeamID);

	UFUNCTION(BlueprintPure)
	ENexusTeamID GetTeamID() const { return TeamID; }

protected:
	UPROPERTY(ReplicatedUsing = OnRep_TeamID, EditAnywhere, BlueprintReadOnly, Category="Team")
	ENexusTeamID TeamID = ENexusTeamID::Neutral;

	UFUNCTION()
	virtual void OnRep_TeamID();
};
