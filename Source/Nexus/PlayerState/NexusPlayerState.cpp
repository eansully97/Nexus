// Fill out your copyright notice in the Description page of Project Settings.


#include "NexusPlayerState.h"

#include "Net/UnrealNetwork.h"

ANexusPlayerState::ANexusPlayerState()
{
}

void ANexusPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANexusPlayerState, TeamID);
}

void ANexusPlayerState::SetTeamID(ENexusTeamID NewTeamID)
{
	TeamID = NewTeamID;
	OnRep_TeamID();
}

void ANexusPlayerState::OnRep_TeamID()
{
	// Optional:
	// update UI, debug text, etc.
}