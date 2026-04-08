#include "Nexus/HUD/Widgets/NexusReconnectPromptWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Nexus/GameInstance/NexusGameInstance.h"

void UNexusReconnectPromptWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	BuildFallbackWidgetTree();
}

void UNexusReconnectPromptWidget::NativeConstruct()
{
	Super::NativeConstruct();

	BindButtonHandlers();

	if (UNexusGameInstance* GameInstance = GetNexusGameInstance())
	{
		GameInstance->OnReconnectStateChanged.RemoveDynamic(this, &ThisClass::HandleReconnectStateChanged);
		GameInstance->OnReconnectStateChanged.AddDynamic(this, &ThisClass::HandleReconnectStateChanged);
	}

	RefreshFromGameInstance();
}

void UNexusReconnectPromptWidget::NativeDestruct()
{
	UnbindButtonHandlers();

	if (UNexusGameInstance* GameInstance = GetNexusGameInstance())
	{
		GameInstance->OnReconnectStateChanged.RemoveDynamic(this, &ThisClass::HandleReconnectStateChanged);
	}

	Super::NativeDestruct();
}

void UNexusReconnectPromptWidget::RefreshFromGameInstance()
{
	if (TitleText)
	{
		TitleText->SetText(NSLOCTEXT("NexusReconnectPrompt", "Title", "Reconnect To Previous Match?"));
	}

	UNexusGameInstance* GameInstance = GetNexusGameInstance();
	if (!GameInstance)
	{
		if (StatusText)
		{
			StatusText->SetText(NSLOCTEXT("NexusReconnectPrompt", "MissingGameInstance", "Reconnect is unavailable right now."));
		}

		if (ReconnectButton)
		{
			ReconnectButton->SetIsEnabled(false);
		}

		if (DismissButton)
		{
			DismissButton->SetIsEnabled(false);
		}

		return;
	}

	if (StatusText)
	{
		StatusText->SetText(GameInstance->GetReconnectStatusText());
	}

	if (ReconnectButton)
	{
		ReconnectButton->SetIsEnabled(
			GameInstance->CanReconnectToLastSession() &&
			!GameInstance->IsReconnectAttemptInProgress());
	}

	if (DismissButton)
	{
		DismissButton->SetIsEnabled(!GameInstance->IsReconnectAttemptInProgress());
	}
}

void UNexusReconnectPromptWidget::HandleReconnectClicked()
{
	if (UNexusGameInstance* GameInstance = GetNexusGameInstance())
	{
		GameInstance->AttemptReconnectToLastSession();
	}
}

void UNexusReconnectPromptWidget::HandleDismissClicked()
{
	if (UNexusGameInstance* GameInstance = GetNexusGameInstance())
	{
		GameInstance->DismissReconnectPrompt();
	}
}

void UNexusReconnectPromptWidget::HandleReconnectStateChanged()
{
	RefreshFromGameInstance();
}

void UNexusReconnectPromptWidget::BuildFallbackWidgetTree()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UOverlay* RootOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("ReconnectRoot"));
	WidgetTree->RootWidget = RootOverlay;

	UBorder* PanelBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ReconnectPanel"));
	PanelBorder->SetPadding(FMargin(20.0f));
	PanelBorder->SetBrushColor(FLinearColor(0.04f, 0.04f, 0.05f, 0.94f));

	if (UOverlaySlot* OverlaySlot = RootOverlay->AddChildToOverlay(PanelBorder))
	{
		OverlaySlot->SetHorizontalAlignment(HAlign_Center);
		OverlaySlot->SetVerticalAlignment(VAlign_Center);
		OverlaySlot->SetPadding(FMargin(24.0f));
	}

	USizeBox* SizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ReconnectSizeBox"));
	SizeBox->SetWidthOverride(520.0f);
	PanelBorder->SetContent(SizeBox);

	UVerticalBox* ContentBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ReconnectContentBox"));
	SizeBox->SetContent(ContentBox);

	TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
	TitleText->SetText(NSLOCTEXT("NexusReconnectPrompt", "TitleDefault", "Reconnect To Previous Match?"));
	TitleText->SetAutoWrapText(true);

	if (UVerticalBoxSlot* TitleSlot = ContentBox->AddChildToVerticalBox(TitleText))
	{
		TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
	}

	StatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StatusText"));
	StatusText->SetAutoWrapText(true);
	StatusText->SetText(NSLOCTEXT("NexusReconnectPrompt", "StatusDefault", "You were disconnected from a recent session."));

	if (UVerticalBoxSlot* StatusSlot = ContentBox->AddChildToVerticalBox(StatusText))
	{
		StatusSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 18.0f));
	}

	UHorizontalBox* ButtonRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ButtonRow"));

	if (UVerticalBoxSlot* ButtonRowSlot = ContentBox->AddChildToVerticalBox(ButtonRow))
	{
		ButtonRowSlot->SetHorizontalAlignment(HAlign_Right);
	}

	ReconnectButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ReconnectButton"));
	ReconnectButton->SetToolTipText(NSLOCTEXT("NexusReconnectPrompt", "ReconnectTooltip", "Try to rejoin the last session."));

	UTextBlock* ReconnectLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ReconnectLabel"));
	ReconnectLabel->SetText(NSLOCTEXT("NexusReconnectPrompt", "ReconnectLabel", "Reconnect"));
	ReconnectButton->SetContent(ReconnectLabel);

	if (UHorizontalBoxSlot* ReconnectSlot = ButtonRow->AddChildToHorizontalBox(ReconnectButton))
	{
		ReconnectSlot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));
	}

	DismissButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("DismissButton"));
	DismissButton->SetToolTipText(NSLOCTEXT("NexusReconnectPrompt", "DismissTooltip", "Close this prompt and stay on the menu."));

	UTextBlock* DismissLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DismissLabel"));
	DismissLabel->SetText(NSLOCTEXT("NexusReconnectPrompt", "DismissLabel", "Dismiss"));
	DismissButton->SetContent(DismissLabel);

	ButtonRow->AddChildToHorizontalBox(DismissButton);
}

void UNexusReconnectPromptWidget::BindButtonHandlers()
{
	if (ReconnectButton)
	{
		ReconnectButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleReconnectClicked);
		ReconnectButton->OnClicked.AddDynamic(this, &ThisClass::HandleReconnectClicked);
	}

	if (DismissButton)
	{
		DismissButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleDismissClicked);
		DismissButton->OnClicked.AddDynamic(this, &ThisClass::HandleDismissClicked);
	}
}

void UNexusReconnectPromptWidget::UnbindButtonHandlers()
{
	if (ReconnectButton)
	{
		ReconnectButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleReconnectClicked);
	}

	if (DismissButton)
	{
		DismissButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleDismissClicked);
	}
}

UNexusGameInstance* UNexusReconnectPromptWidget::GetNexusGameInstance() const
{
	return GetWorld() ? GetWorld()->GetGameInstance<UNexusGameInstance>() : nullptr;
}
