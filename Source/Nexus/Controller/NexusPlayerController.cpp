// Fill out your copyright notice in the Description page of Project Settings.


#include "NexusPlayerController.h"

#include "EnhancedInputSubsystems.h"
#include "Nexus/HUD/NexusHUD.h"
#include "Nexus/HUD/Widgets/NexusMainHUDWidget.h"

void ANexusPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// Add Input Mapping Contexts
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
		{
			Subsystem->AddMappingContext(CurrentContext, 0);
		}
	}
}

void ANexusPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	RefreshHUDBindings();
}

void ANexusPlayerController::OnRep_Pawn()
{
	Super::OnRep_Pawn();
	RefreshHUDBindings();
}

void ANexusPlayerController::RefreshHUDBindings()
{
	if (!IsLocalController())
	{
		return;
	}

	ANexusHUD* NexusHUD = Cast<ANexusHUD>(GetHUD());
	if (!NexusHUD)
	{
		return;
	}

	UNexusMainHUDWidget* MainWidget = NexusHUD->GetMainHUDWidget();
	if (!MainWidget)
	{
		return;
	}

	MainWidget->SetObservedPawn(GetPawn());
}
