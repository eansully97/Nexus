// Fill out your copyright notice in the Description page of Project Settings.


#include "ClassSelectionWidget.h"

#include "Nexus/Controller/NexusPlayerController.h"

void UClassSelectionWidget::SelectClass(UCharacterClassInfo* InClassInfo)
{
	if (ANexusPlayerController* PC = GetOwningPlayer<ANexusPlayerController>())
	{
		PC->Server_SelectClass(InClassInfo);
	}
}

void UClassSelectionWidget::ClickReady()
{
	if (ANexusPlayerController* PC = GetOwningPlayer<ANexusPlayerController>())
	{
		PC->Server_SetReady(true);
	}
}