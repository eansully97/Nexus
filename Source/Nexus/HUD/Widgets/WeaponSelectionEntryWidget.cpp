#include "Nexus/HUD/Widgets/WeaponSelectionEntryWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Nexus/Weapons/NexusWeaponBase.h"

void UWeaponSelectionEntryWidget::NativeConstruct()
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

void UWeaponSelectionEntryWidget::NativeDestruct()
{
	if (EntryButton)
	{
		EntryButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleEntryButtonClicked);
	}

	Super::NativeDestruct();
}

void UWeaponSelectionEntryWidget::InitializeWeaponEntry(TSubclassOf<ANexusWeaponBase> InWeaponClass)
{
	WeaponClass = InWeaponClass;
	RefreshEntryText();
	RefreshEntryState();
}

void UWeaponSelectionEntryWidget::SetEntrySelected(bool bInSelected)
{
	if (bSelected == bInSelected)
	{
		return;
	}

	bSelected = bInSelected;
	RefreshEntryState();
}

void UWeaponSelectionEntryWidget::SetEntryInteractionEnabled(bool bInEnabled)
{
	if (bInteractionEnabled == bInEnabled)
	{
		return;
	}

	bInteractionEnabled = bInEnabled;
	RefreshEntryState();
}

void UWeaponSelectionEntryWidget::HandleEntryButtonClicked()
{
	if (!bInteractionEnabled || !WeaponClass)
	{
		return;
	}

	OnWeaponChosen.Broadcast(WeaponClass);
}

void UWeaponSelectionEntryWidget::RefreshEntryText()
{
	if (WeaponNameText)
	{
		WeaponNameText->SetText(GetWeaponDisplayText());
	}
}

void UWeaponSelectionEntryWidget::RefreshEntryState()
{
	if (EntryButton)
	{
		EntryButton->SetIsEnabled(bInteractionEnabled);
	}

	BP_OnEntryVisualStateChanged(bSelected, bInteractionEnabled);
}

FText UWeaponSelectionEntryWidget::GetWeaponDisplayText() const
{
	if (!WeaponClass)
	{
		return FText::GetEmpty();
	}

	FString ClassName = WeaponClass->GetName();
	ClassName.RemoveFromEnd(TEXT("_C"));
	ClassName.RemoveFromStart(TEXT("BP_"));

	return FText::FromString(ClassName);
}