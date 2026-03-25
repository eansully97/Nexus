// Fill out your copyright notice in the Description page of Project Settings.


#include "NexusGameState.h"

#include "Net/UnrealNetwork.h"

void ANexusGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ThisClass, TeamAScore)
	DOREPLIFETIME(ThisClass, TeamBScore)
}
