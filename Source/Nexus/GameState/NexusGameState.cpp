// Fill out your copyright notice in the Description page of Project Settings.


#include "NexusGameState.h"

#include "Net/UnrealNetwork.h"
#include "Nexus/Controller/NexusPlayerController.h"

void ANexusGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ThisClass, TeamAScore)
	DOREPLIFETIME(ThisClass, TeamBScore)
	DOREPLIFETIME(ThisClass, bClassSelectOpen);
	DOREPLIFETIME(ThisClass, ReadyPlayerCount);
}

void ANexusGameState::OnRep_ClassSelectOpen()
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (ANexusPlayerController* PC = Cast<ANexusPlayerController>(It->Get()))
		{
			if (bClassSelectOpen)
			{
				PC->ShowClassSelectUI();
			}
			else
			{
				PC->HideClassSelectUI();
			}
		}
	}
}