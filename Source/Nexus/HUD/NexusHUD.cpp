// Fill out your copyright notice in the Description page of Project Settings.


#include "NexusHUD.h"

#include "Blueprint/UserWidget.h"
#include "Widgets/NexusMainHUDWidget.h"

void ANexusHUD::BeginPlay()
{
	Super::BeginPlay();

	
}

void ANexusHUD::InitMainHUDWidget()
{
	if (MainHUDWidget || !MainHUDWidgetClass)
	{
		return;
	}

	if (PlayerOwner)
	{
		MainHUDWidget = CreateWidget<UNexusMainHUDWidget>(PlayerOwner, MainHUDWidgetClass);
		if (MainHUDWidget)
		{
			MainHUDWidget->AddToViewport();
		}
	}
}
