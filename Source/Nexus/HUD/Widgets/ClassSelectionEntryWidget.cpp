#include "Nexus/HUD/Widgets/ClassSelectionEntryWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Nexus/DataAssets/CharacterClassInfo.h"

void UClassSelectionEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (EntryButton)
	{
		EntryButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleEntryButtonClicked);
		EntryButton->OnClicked.AddDynamic(this, &ThisClass::HandleEntryButtonClicked);
	}

	RefreshEntryText();
	RefreshEntryState();
}

void UClassSelectionEntryWidget::NativeDestruct()
{
	if (EntryButton)
	{
		EntryButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleEntryButtonClicked);
	}

	Super::NativeDestruct();
}

void UClassSelectionEntryWidget::InitializeClassEntry(UCharacterClassInfo* InClassInfo)
{
	ClassInfo = InClassInfo;
	RefreshEntryText();
	RefreshEntryState();
}

void UClassSelectionEntryWidget::SetEntrySelected(bool bInSelected)
{
	if (bSelected == bInSelected)
	{
		return;
	}

	bSelected = bInSelected;
	RefreshEntryState();
}

void UClassSelectionEntryWidget::SetEntryInteractionEnabled(bool bInEnabled)
{
	if (bInteractionEnabled == bInEnabled)
	{
		return;
	}

	bInteractionEnabled = bInEnabled;
	RefreshEntryState();
}

void UClassSelectionEntryWidget::HandleEntryButtonClicked()
{
	if (!bInteractionEnabled || !ClassInfo)
	{
		return;
	}

	OnClassChosen.Broadcast(ClassInfo);
}

void UClassSelectionEntryWidget::RefreshEntryText()
{
	if (ClassNameText)
	{
		ClassNameText->SetText(GetClassDisplayText());
	}
}

void UClassSelectionEntryWidget::RefreshEntryState()
{
	if (EntryButton)
	{
		EntryButton->SetIsEnabled(bInteractionEnabled);
	}

	BP_OnEntryVisualStateChanged(bSelected, bInteractionEnabled);
}

FText UClassSelectionEntryWidget::GetClassDisplayText() const
{
	if (!ClassInfo)
	{
		return FText::GetEmpty();
	}

	FString AssetName = ClassInfo->GetName();
	AssetName.RemoveFromStart(TEXT("DA_"));
	return FText::FromString(AssetName);
}