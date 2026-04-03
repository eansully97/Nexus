#include "ANexusMenuPlayerController.h"

#include "Blueprint/UserWidget.h"

ANexusMenuPlayerController::ANexusMenuPlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
}

void ANexusMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalController())
	{
		ShowMainMenu();
	}
}

void ANexusMenuPlayerController::ShowMainMenu()
{
	if (!IsLocalController())
	{
		return;
	}

	if (MainMenuWidget)
	{
		return;
	}

	if (!MainMenuWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("NexusMenuPlayerController: MainMenuWidgetClass is null"));
		return;
	}

	MainMenuWidget = CreateWidget<UUserWidget>(this, MainMenuWidgetClass);
	if (!MainMenuWidget)
	{
		UE_LOG(LogTemp, Warning, TEXT("NexusMenuPlayerController: Failed to create MainMenuWidget"));
		return;
	}

	MainMenuWidget->AddToViewport();
	ApplyMenuInputMode();
}

void ANexusMenuPlayerController::HideMainMenu()
{
	if (!IsLocalController())
	{
		return;
	}

	if (MainMenuWidget)
	{
		MainMenuWidget->RemoveFromParent();
		MainMenuWidget = nullptr;
	}

	bShowMouseCursor = false;

	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);
}

void ANexusMenuPlayerController::ApplyMenuInputMode()
{
	bShowMouseCursor = true;

	FInputModeUIOnly InputMode;
	if (MainMenuWidget)
	{
		InputMode.SetWidgetToFocus(MainMenuWidget->TakeWidget());
	}
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
}