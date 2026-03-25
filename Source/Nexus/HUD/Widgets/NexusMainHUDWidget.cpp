// Fill out your copyright notice in the Description page of Project Settings.


#include "NexusMainHUDWidget.h"

#include "NexusVitalsWidget.h"
#include "Nexus/Character/NexusCharacterBase.h"

void UNexusMainHUDWidget::SetObservedPawn(APawn* NewPawn)
{
	ObservedCharacter = Cast<ANexusCharacterBase>(NewPawn);
	if (VitalsWidget)
	{
		VitalsWidget->SetObservedPawn(NewPawn);
	}
	// Later:
	// Unbind old delegates
	// Bind new ASC / attribute delegates
	// Refresh health, stamina, ammo, etc.
}
