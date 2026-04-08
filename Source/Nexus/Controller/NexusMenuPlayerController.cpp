#include "NexusMenuPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Nexus/GameInstance/NexusGameInstance.h"
#include "Nexus/HUD/Widgets/NexusLocalTestQuickJoinWidget.h"
#include "Nexus/HUD/Widgets/NexusReconnectPromptWidget.h"
#include "Widgets/SWidget.h"

ANexusMenuPlayerController::ANexusMenuPlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
	LocalTestQuickJoinWidgetClass = UNexusLocalTestQuickJoinWidget::StaticClass();
	ReconnectPromptWidgetClass = UNexusReconnectPromptWidget::StaticClass();
}

void ANexusMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController())
	{
		return;
	}

	if (UNexusGameInstance* GameInstance = GetNexusGameInstance())
	{
		GameInstance->OnReconnectStateChanged.RemoveDynamic(this, &ThisClass::HandleReconnectStateChanged);
		GameInstance->OnReconnectStateChanged.AddDynamic(this, &ThisClass::HandleReconnectStateChanged);
	}

	ShowMainMenu();
	RefreshReconnectPrompt();
	RefreshLocalTestQuickJoinWidget();
}

void ANexusMenuPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UNexusGameInstance* GameInstance = GetNexusGameInstance())
	{
		GameInstance->OnReconnectStateChanged.RemoveDynamic(this, &ThisClass::HandleReconnectStateChanged);
	}

	if (ReconnectPromptWidget)
	{
		ReconnectPromptWidget->RemoveFromParent();
		ReconnectPromptWidget = nullptr;
	}

	if (LocalTestQuickJoinWidget)
	{
		LocalTestQuickJoinWidget->RemoveFromParent();
		LocalTestQuickJoinWidget = nullptr;
	}

	Super::EndPlay(EndPlayReason);
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
	ApplyMenuInputMode(MainMenuWidget);
	RefreshLocalTestQuickJoinWidget();
}

void ANexusMenuPlayerController::HideMainMenu()
{
	if (!IsLocalController())
	{
		return;
	}

	if (ReconnectPromptWidget)
	{
		ReconnectPromptWidget->RemoveFromParent();
		ReconnectPromptWidget = nullptr;
	}

	if (LocalTestQuickJoinWidget)
	{
		LocalTestQuickJoinWidget->RemoveFromParent();
		LocalTestQuickJoinWidget = nullptr;
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

void ANexusMenuPlayerController::RefreshReconnectPrompt()
{
	if (!IsLocalController())
	{
		return;
	}

	UNexusGameInstance* GameInstance = GetNexusGameInstance();
	const bool bShouldShowPrompt = GameInstance && GameInstance->ShouldShowReconnectPrompt();

	if (!bShouldShowPrompt)
	{
		if (ReconnectPromptWidget)
		{
			ReconnectPromptWidget->RemoveFromParent();
			ReconnectPromptWidget = nullptr;
		}

		ApplyMenuInputMode(MainMenuWidget);
		return;
	}

	TSubclassOf<UNexusReconnectPromptWidget> PromptClass = ReconnectPromptWidgetClass;
	if (!PromptClass)
	{
		PromptClass = UNexusReconnectPromptWidget::StaticClass();
	}

	if (!ReconnectPromptWidget)
	{
		ReconnectPromptWidget = CreateWidget<UNexusReconnectPromptWidget>(this, PromptClass);
		if (!ReconnectPromptWidget)
		{
			UE_LOG(LogTemp, Warning, TEXT("NexusMenuPlayerController: Failed to create ReconnectPromptWidget"));
			return;
		}

		ReconnectPromptWidget->AddToViewport(20);
	}

	ReconnectPromptWidget->RefreshFromGameInstance();
	ApplyMenuInputMode(ReconnectPromptWidget);
}

void ANexusMenuPlayerController::HandleReconnectStateChanged()
{
	RefreshReconnectPrompt();
	RefreshLocalTestQuickJoinWidget();
}

UNexusGameInstance* ANexusMenuPlayerController::GetNexusGameInstance() const
{
	return GetGameInstance<UNexusGameInstance>();
}

void ANexusMenuPlayerController::RefreshLocalTestQuickJoinWidget()
{
	if (!IsLocalController())
	{
		return;
	}

	UNexusGameInstance* GameInstance = GetNexusGameInstance();
	const bool bIsOnMainMenuMap =
		GameInstance &&
		UGameplayStatics::GetCurrentLevelName(this, true) == GameInstance->MainMenuMapName.ToString();
	const bool bShouldShowWidget =
		GameInstance &&
		GameInstance->IsUsingLocalTestSessions() &&
		(MainMenuWidget != nullptr || bIsOnMainMenuMap);

	if (!bShouldShowWidget)
	{
		if (LocalTestQuickJoinWidget)
		{
			LocalTestQuickJoinWidget->RemoveFromParent();
			LocalTestQuickJoinWidget = nullptr;
		}

		return;
	}

	TSubclassOf<UNexusLocalTestQuickJoinWidget> WidgetClass = LocalTestQuickJoinWidgetClass;
	if (!WidgetClass)
	{
		WidgetClass = UNexusLocalTestQuickJoinWidget::StaticClass();
	}

	if (!LocalTestQuickJoinWidget)
	{
		LocalTestQuickJoinWidget = CreateWidget<UNexusLocalTestQuickJoinWidget>(this, WidgetClass);
		if (!LocalTestQuickJoinWidget)
		{
			UE_LOG(LogTemp, Warning, TEXT("NexusMenuPlayerController: Failed to create LocalTestQuickJoinWidget"));
			return;
		}

		LocalTestQuickJoinWidget->AddToViewport(15);
	}

	LocalTestQuickJoinWidget->RefreshFromGameInstance();
}

void ANexusMenuPlayerController::ApplyMenuInputMode(UUserWidget* FocusWidget)
{
	bShowMouseCursor = true;

	FInputModeUIOnly InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);

	UUserWidget* WidgetToFocus = FocusWidget ? FocusWidget : MainMenuWidget.Get();
	if (WidgetToFocus)
	{
		const TSharedPtr<SWidget> SlateWidget = WidgetToFocus->GetCachedWidget();
		if (SlateWidget.IsValid() && SlateWidget->SupportsKeyboardFocus())
		{
			InputMode.SetWidgetToFocus(SlateWidget);
		}
	}

	SetInputMode(InputMode);
}
