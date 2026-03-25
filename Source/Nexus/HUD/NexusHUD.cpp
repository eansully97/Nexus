// Fill out your copyright notice in the Description page of Project Settings.


#include "NexusHUD.h"

#include "Blueprint/UserWidget.h"
#include "Widgets/NexusMainHUDWidget.h"

void ANexusHUD::BeginPlay()
{
	Super::BeginPlay();

	if (MainHUDWidgetClass)
	{
		MainHUDWidget = CreateWidget<UNexusMainHUDWidget>(GetWorld(), MainHUDWidgetClass);
		if (MainHUDWidget)
		{
			MainHUDWidget->AddToViewport();
		}
	}
}
