#include "Nexus/HUD/Widgets/NexusCharacterOverheadWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "Nexus/Character/NexusCharacterBase.h"
#include "Nexus/Character/Player/NexusPlayerCharacter.h"
#include "Styling/SlateColor.h"

void UNexusCharacterOverheadWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RefreshFromCharacter();
}

void UNexusCharacterOverheadWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	BuildFallbackWidgetTree();
}

void UNexusCharacterOverheadWidget::NativeDestruct()
{
	UnbindFromObservedCharacter();
	Super::NativeDestruct();
}

void UNexusCharacterOverheadWidget::SetObservedCharacter(ANexusCharacterBase* NewCharacter)
{
	if (ObservedCharacter == NewCharacter)
	{
		RefreshFromCharacter();
		return;
	}

	UnbindFromObservedCharacter();
	SetObservedPawn(NewCharacter);
	BindToObservedCharacter();
	RefreshFromCharacter();
}

void UNexusCharacterOverheadWidget::HandleObservedTeamChanged()
{
	RefreshTeamPresentation();
}

void UNexusCharacterOverheadWidget::BindToObservedCharacter()
{
	if (!ObservedCharacter)
	{
		return;
	}

	ObservedCharacter->OnTeamChanged.RemoveDynamic(this, &ThisClass::HandleObservedTeamChanged);
	ObservedCharacter->OnTeamChanged.AddDynamic(this, &ThisClass::HandleObservedTeamChanged);
}

void UNexusCharacterOverheadWidget::UnbindFromObservedCharacter()
{
	if (ObservedCharacter)
	{
		ObservedCharacter->OnTeamChanged.RemoveDynamic(this, &ThisClass::HandleObservedTeamChanged);
	}
}

void UNexusCharacterOverheadWidget::RefreshFromCharacter()
{
	const ANexusPlayerCharacter* ObservedPlayerCharacter = Cast<ANexusPlayerCharacter>(ObservedCharacter);
	const bool bShouldHideForOwner = ObservedPlayerCharacter && ObservedPlayerCharacter->IsLocallyControlled();
	SetVisibility(bShouldHideForOwner ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);

	if (ObservedASC)
	{
		RefreshFromAttributes();
	}
	else
	{
		Health = 0.0f;
		MaxHealth = 0.0f;
		Stamina = 0.0f;
		MaxStamina = 0.0f;
	}

	RefreshText();
	RefreshHealthBar();
	RefreshLayout();
	RefreshTeamPresentation();
}

void UNexusCharacterOverheadWidget::RefreshText()
{
	if (!NameText)
	{
		return;
	}

	const ANexusPlayerCharacter* PlayerCharacter = Cast<ANexusPlayerCharacter>(ObservedCharacter);
	if (!PlayerCharacter)
	{
		NameText->SetVisibility(ESlateVisibility::Collapsed);
		NameText->SetText(FText::GetEmpty());
		return;
	}

	const APlayerState* PlayerState = PlayerCharacter->GetPlayerState();
	const FString PlayerName = PlayerState ? PlayerState->GetPlayerName() : FString();

	if (PlayerName.IsEmpty())
	{
		NameText->SetVisibility(ESlateVisibility::Collapsed);
		NameText->SetText(FText::GetEmpty());
		return;
	}

	NameText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	NameText->SetText(FText::FromString(PlayerName));
}

void UNexusCharacterOverheadWidget::RefreshHealthBar()
{
	if (!HealthBar)
	{
		return;
	}

	const float HealthPercent = MaxHealth > 0.0f ? Health / MaxHealth : 0.0f;
	HealthBar->SetPercent(FMath::Clamp(HealthPercent, 0.0f, 1.0f));
}

void UNexusCharacterOverheadWidget::RefreshLayout()
{
	if (!HealthBar)
	{
		return;
	}

	const bool bIsPlayerCharacter = Cast<ANexusPlayerCharacter>(ObservedCharacter) != nullptr;
	const float WidthScale = bIsPlayerCharacter ? PlayerHealthBarWidthScale : MinionHealthBarWidthScale;
	const float HeightScale = bIsPlayerCharacter ? PlayerHealthBarHeightScale : MinionHealthBarHeightScale;

	HealthBar->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	HealthBar->SetRenderScale(FVector2D(WidthScale, HeightScale));

	if (RootBorder)
	{
		const FMargin DesiredPadding = bIsPlayerCharacter
			? FMargin(PlayerHorizontalPadding, PlayerVerticalPadding)
			: FMargin(MinionHorizontalPadding, MinionVerticalPadding);
		RootBorder->SetPadding(DesiredPadding);
	}
}

void UNexusCharacterOverheadWidget::RefreshTeamPresentation()
{
	if (!ObservedCharacter)
	{
		if (HealthBar)
		{
			HealthBar->SetFillColorAndOpacity(NeutralHealthColor);
		}

		if (RootBorder)
		{
			RootBorder->SetBrushColor(FLinearColor(0.02f, 0.02f, 0.02f, 0.65f));
		}

		return;
	}

	FLinearColor TeamColor = NeutralHealthColor;
	if (const ANexusPlayerCharacter* PlayerCharacter = Cast<ANexusPlayerCharacter>(ObservedCharacter))
	{
		switch (PlayerCharacter->GetTeamID())
		{
		case ENexusTeamID::TeamA:
			TeamColor = TeamAHealthColor;
			break;

		case ENexusTeamID::TeamB:
			TeamColor = TeamBHealthColor;
			break;

		default:
			break;
		}
	}
	else
	{
		const ANexusCharacterBase* LocalCharacter =
			Cast<ANexusCharacterBase>(UGameplayStatics::GetPlayerPawn(this, 0));
		if (!LocalCharacter || LocalCharacter->GetTeamID() == ENexusTeamID::Neutral)
		{
			TeamColor = NeutralHealthColor;
		}
		else if (ObservedCharacter->GetTeamID() == LocalCharacter->GetTeamID())
		{
			TeamColor = AllyHealthColor;
		}
		else if (ObservedCharacter->GetTeamID() != ENexusTeamID::Neutral)
		{
			TeamColor = EnemyHealthColor;
		}
	}

	if (HealthBar)
	{
		HealthBar->SetFillColorAndOpacity(TeamColor);
	}

	if (NameText && NameText->GetVisibility() != ESlateVisibility::Collapsed)
	{
		NameText->SetColorAndOpacity(FSlateColor(TeamColor));
	}

	if (RootBorder)
	{
		RootBorder->SetBrushColor(FLinearColor(0.02f, 0.02f, 0.02f, 0.65f));
	}
}

void UNexusCharacterOverheadWidget::BuildFallbackWidgetTree()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	RootBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RootBorder"));
	RootBorder->SetPadding(FMargin(PlayerHorizontalPadding, PlayerVerticalPadding));
	RootBorder->SetBrushColor(FLinearColor(0.02f, 0.02f, 0.02f, 0.65f));
	WidgetTree->RootWidget = RootBorder;

	UVerticalBox* ContentBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ContentBox"));
	RootBorder->SetContent(ContentBox);

	NameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("NameText"));
	NameText->SetJustification(ETextJustify::Center);
	NameText->SetVisibility(ESlateVisibility::Collapsed);

	if (UVerticalBoxSlot* NameSlot = ContentBox->AddChildToVerticalBox(NameText))
	{
		NameSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 4.0f));
		NameSlot->SetHorizontalAlignment(HAlign_Center);
	}

	HealthBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("HealthBar"));
	HealthBar->SetPercent(1.0f);

	if (UVerticalBoxSlot* HealthSlot = ContentBox->AddChildToVerticalBox(HealthBar))
	{
		HealthSlot->SetHorizontalAlignment(HAlign_Fill);
	}
}
