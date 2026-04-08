#include "Nexus/HUD/Widgets/NexusLocalTestQuickJoinWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Nexus/GameInstance/NexusGameInstance.h"

void UNexusLocalTestQuickJoinWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	BuildFallbackWidgetTree();
}

void UNexusLocalTestQuickJoinWidget::NativeConstruct()
{
	Super::NativeConstruct();

	BindButtonHandlers();
	RefreshFromGameInstance();
}

void UNexusLocalTestQuickJoinWidget::NativeDestruct()
{
	UnbindButtonHandlers();

	Super::NativeDestruct();
}

void UNexusLocalTestQuickJoinWidget::RefreshFromGameInstance()
{
	if (TitleText)
	{
		TitleText->SetText(NSLOCTEXT("NexusLocalTestQuickJoin", "Title", "Local Test Mode"));
	}

	UNexusGameInstance* GameInstance = GetNexusGameInstance();
	if (!GameInstance)
	{
		if (StatusText)
		{
			StatusText->SetText(NSLOCTEXT("NexusLocalTestQuickJoin", "MissingGameInstance", "Game instance unavailable."));
		}

		if (JoinButton)
		{
			JoinButton->SetIsEnabled(false);
		}

		return;
	}

	if (StatusText)
	{
		StatusText->SetText(NSLOCTEXT(
			"NexusLocalTestQuickJoin",
			"Status",
			"Host in the other local window, then use this to connect directly to 127.0.0.1:7777."));
	}

	if (JoinButton)
	{
		JoinButton->SetIsEnabled(GameInstance->IsUsingLocalTestSessions() && !GameInstance->bSessionActionInProgress);
	}
}

void UNexusLocalTestQuickJoinWidget::HandleJoinClicked()
{
	if (UNexusGameInstance* GameInstance = GetNexusGameInstance())
	{
		GameInstance->JoinLocalTestServer();
	}
}

void UNexusLocalTestQuickJoinWidget::BuildFallbackWidgetTree()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UOverlay* RootOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("LocalTestRoot"));
	WidgetTree->RootWidget = RootOverlay;

	UBorder* PanelBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("LocalTestPanel"));
	PanelBorder->SetPadding(FMargin(14.0f));
	PanelBorder->SetBrushColor(FLinearColor(0.05f, 0.09f, 0.12f, 0.9f));

	if (UOverlaySlot* OverlaySlot = RootOverlay->AddChildToOverlay(PanelBorder))
	{
		OverlaySlot->SetHorizontalAlignment(HAlign_Left);
		OverlaySlot->SetVerticalAlignment(VAlign_Top);
		OverlaySlot->SetPadding(FMargin(24.0f));
	}

	USizeBox* SizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("LocalTestSizeBox"));
	SizeBox->SetWidthOverride(360.0f);
	PanelBorder->SetContent(SizeBox);

	UVerticalBox* ContentBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("LocalTestContent"));
	SizeBox->SetContent(ContentBox);

	TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
	TitleText->SetText(NSLOCTEXT("NexusLocalTestQuickJoin", "TitleDefault", "Local Test Mode"));
	TitleText->SetAutoWrapText(true);

	if (UVerticalBoxSlot* TitleSlot = ContentBox->AddChildToVerticalBox(TitleText))
	{
		TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));
	}

	StatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StatusText"));
	StatusText->SetAutoWrapText(true);
	StatusText->SetText(NSLOCTEXT(
		"NexusLocalTestQuickJoin",
		"StatusDefault",
		"Host in the other local window, then use this button to join directly."));

	if (UVerticalBoxSlot* StatusSlot = ContentBox->AddChildToVerticalBox(StatusText))
	{
		StatusSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
	}

	JoinButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("JoinButton"));
	JoinButton->SetToolTipText(NSLOCTEXT("NexusLocalTestQuickJoin", "JoinTooltip", "Connect directly to the local listen server."));

	UTextBlock* JoinLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("JoinLabel"));
	JoinLabel->SetText(NSLOCTEXT("NexusLocalTestQuickJoin", "JoinLabel", "Join Local Test"));
	JoinButton->SetContent(JoinLabel);

	ContentBox->AddChildToVerticalBox(JoinButton);
}

void UNexusLocalTestQuickJoinWidget::BindButtonHandlers()
{
	if (JoinButton)
	{
		JoinButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleJoinClicked);
		JoinButton->OnClicked.AddDynamic(this, &ThisClass::HandleJoinClicked);
	}
}

void UNexusLocalTestQuickJoinWidget::UnbindButtonHandlers()
{
	if (JoinButton)
	{
		JoinButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleJoinClicked);
	}
}

UNexusGameInstance* UNexusLocalTestQuickJoinWidget::GetNexusGameInstance() const
{
	return GetWorld() ? GetWorld()->GetGameInstance<UNexusGameInstance>() : nullptr;
}
