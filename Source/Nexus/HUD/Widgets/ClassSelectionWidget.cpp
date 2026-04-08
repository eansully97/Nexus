#include "Nexus/HUD/Widgets/ClassSelectionWidget.h"

#include "Components/Button.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "Nexus/Controller/NexusPlayerController.h"
#include "Nexus/DataAssets/CharacterClassInfo.h"
#include "Nexus/GameState/NexusGameState.h"
#include "Nexus/HUD/Widgets/ClassSelectionEntryWidget.h"
#include "Nexus/HUD/Widgets/WeaponSelectionEntryWidget.h"
#include "Nexus/PlayerState/NexusPlayerState.h"
#include "Nexus/Weapons/NexusWeaponBase.h"

void UClassSelectionWidget::NativeConstruct()
{
	Super::NativeConstruct();

	BindNativeButtonHandlers();
	BindToObservedPlayerState();
	BindToObservedGameState();
	BuildClassPage();
	RefreshFromObservedState();
}

void UClassSelectionWidget::NativeDestruct()
{
	UnbindNativeButtonHandlers();
	UnbindFromObservedPlayerState();
	UnbindFromObservedGameState();
	ClearClassEntryWidgets();
	ClearWeaponEntryWidgets();

	Super::NativeDestruct();
}

ANexusPlayerController* UClassSelectionWidget::GetNexusPC() const
{
	return GetOwningPlayer<ANexusPlayerController>();
}

ANexusPlayerState* UClassSelectionWidget::ResolvePlayerState() const
{
	if (const ANexusPlayerController* PC = GetNexusPC())
	{
		return PC->GetPlayerState<ANexusPlayerState>();
	}

	return GetOwningPlayerState<ANexusPlayerState>();
}

ANexusGameState* UClassSelectionWidget::ResolveGameState() const
{
	if (const UWorld* World = GetWorld())
	{
		return World->GetGameState<ANexusGameState>();
	}

	return nullptr;
}

void UClassSelectionWidget::BindToObservedPlayerState()
{
	ANexusPlayerState* NewPlayerState = ResolvePlayerState();
	if (ObservedPlayerState == NewPlayerState && ObservedPlayerState)
	{
		return;
	}

	UnbindFromObservedPlayerState();
	ObservedPlayerState = NewPlayerState;

	if (ObservedPlayerState)
	{
		ObservedPlayerState->OnPlayerProfileChanged.AddDynamic(
			this,
			&ThisClass::HandleObservedPlayerProfileChanged);
	}
}

void UClassSelectionWidget::BindToObservedGameState()
{
	ANexusGameState* NewGameState = ResolveGameState();
	if (ObservedGameState == NewGameState && ObservedGameState)
	{
		return;
	}

	UnbindFromObservedGameState();
	ObservedGameState = NewGameState;

	if (ObservedGameState)
	{
		ObservedGameState->OnAvailableClassesChanged.AddDynamic(
			this,
			&ThisClass::HandleObservedAvailableClassesChanged);
	}
}

void UClassSelectionWidget::UnbindFromObservedPlayerState()
{
	if (ObservedPlayerState)
	{
		ObservedPlayerState->OnPlayerProfileChanged.RemoveDynamic(
			this,
			&ThisClass::HandleObservedPlayerProfileChanged);
	}

	ObservedPlayerState = nullptr;
}

void UClassSelectionWidget::UnbindFromObservedGameState()
{
	if (ObservedGameState)
	{
		ObservedGameState->OnAvailableClassesChanged.RemoveDynamic(
			this,
			&ThisClass::HandleObservedAvailableClassesChanged);
	}

	ObservedGameState = nullptr;
}

void UClassSelectionWidget::BindNativeButtonHandlers()
{
	if (ReadyButton)
	{
		ReadyButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleReadyButtonClicked);
		ReadyButton->OnClicked.AddDynamic(this, &ThisClass::HandleReadyButtonClicked);
	}

	if (BackButton)
	{
		BackButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleBackButtonClicked);
		BackButton->OnClicked.AddDynamic(this, &ThisClass::HandleBackButtonClicked);
	}
}

void UClassSelectionWidget::UnbindNativeButtonHandlers()
{
	if (ReadyButton)
	{
		ReadyButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleReadyButtonClicked);
	}

	if (BackButton)
	{
		BackButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleBackButtonClicked);
	}
}

void UClassSelectionWidget::HandleObservedPlayerProfileChanged()
{
	RefreshFromObservedState();
}

void UClassSelectionWidget::HandleObservedAvailableClassesChanged()
{
	BuildClassPage();
	RefreshFromObservedState();
}

void UClassSelectionWidget::HandleReadyButtonClicked()
{
	if (GetIsReadyLockedIn())
	{
		ClickUnready();
		return;
	}

	if (CanClickReady())
	{
		ClickReady();
	}
}

void UClassSelectionWidget::HandleBackButtonClicked()
{
	if (GetIsReadyLockedIn())
	{
		return;
	}

	ShowClassPage();
}

void UClassSelectionWidget::HandleClassEntryChosen(UCharacterClassInfo* InClassInfo)
{
	if (GetIsReadyLockedIn() || !InClassInfo)
	{
		return;
	}

	SelectClass(InClassInfo);
	BuildWeaponPageFromClass(InClassInfo);
	ShowWeaponPage();
}

void UClassSelectionWidget::HandleWeaponEntryChosen(TSubclassOf<ANexusWeaponBase> InWeaponClass)
{
	if (GetIsReadyLockedIn() || !InWeaponClass)
	{
		return;
	}

	SelectWeapon(InWeaponClass);
}

void UClassSelectionWidget::RefreshFromObservedState()
{
	BindToObservedPlayerState();
	BindToObservedGameState();

	if (CurrentClassEntries.IsEmpty() &&
		ObservedGameState &&
		ObservedGameState->GetAvailableClasses().Num() > 0)
	{
		BuildClassPage();
	}

	UCharacterClassInfo* SelectedClass = GetSelectedClass();
	const TSubclassOf<ANexusWeaponBase> SelectedWeapon = GetSelectedWeapon();
	const TArray<FNexusAbilityGrant> SelectedAbilities = GetSelectedClassAbilities();
	const bool bLockedIn = GetIsReadyLockedIn();

	if (SelectedClass)
	{
		if (DisplayedClassInfo != SelectedClass || CurrentWeaponEntries.IsEmpty())
		{
			BuildWeaponPageFromClass(SelectedClass);
		}
	}
	else
	{
		DisplayedClassInfo = nullptr;
		ClearWeaponEntryWidgets();
		ShowClassPage();
	}

	RefreshClassEntryStates(SelectedClass, bLockedIn);
	RefreshWeaponEntryStates(SelectedWeapon, bLockedIn);
	RefreshNativeControlStates(SelectedClass, SelectedWeapon, bLockedIn);

	BP_OnSelectionStateRefreshed(
		SelectedClass,
		SelectedWeapon,
		SelectedAbilities,
		bLockedIn);
}

void UClassSelectionWidget::SelectClass(UCharacterClassInfo* InClassInfo)
{
	if (ANexusPlayerController* PC = GetNexusPC())
	{
		PC->Server_SelectClass(InClassInfo);
	}
}

void UClassSelectionWidget::SelectWeapon(TSubclassOf<ANexusWeaponBase> InWeaponClass)
{
	if (ANexusPlayerController* PC = GetNexusPC())
	{
		PC->Server_SelectWeapon(InWeaponClass);
	}
}

void UClassSelectionWidget::SetSelectedClassAbilities(const TArray<FNexusAbilityGrant>& InAbilityGrants)
{
	if (ANexusPlayerController* PC = GetNexusPC())
	{
		PC->Server_SetSelectedClassAbilities(InAbilityGrants);
	}
}

void UClassSelectionWidget::ClickReady()
{
	if (ANexusPlayerController* PC = GetNexusPC())
	{
		PC->Server_SetReady(true);
	}
}

void UClassSelectionWidget::ClickUnready()
{
	if (ANexusPlayerController* PC = GetNexusPC())
	{
		PC->Server_SetReady(false);
	}
}

void UClassSelectionWidget::BuildClassPage()
{
	RebuildClassEntryWidgets();
	RefreshClassEntryStates(GetSelectedClass(), GetIsReadyLockedIn());
}

void UClassSelectionWidget::BuildWeaponPageFromClass(UCharacterClassInfo* InClassInfo)
{
	DisplayedClassInfo = InClassInfo;

	if (SelectedClassText)
	{
		SelectedClassText->SetText(GetClassDisplayText(InClassInfo));
	}

	RebuildWeaponEntryWidgets(InClassInfo);
	RefreshWeaponEntryStates(GetSelectedWeapon(), GetIsReadyLockedIn());
}

void UClassSelectionWidget::ShowClassPage()
{
	if (WidgetSwitcher_1)
	{
		WidgetSwitcher_1->SetActiveWidgetIndex(ClassPageIndex);
	}
}

void UClassSelectionWidget::ShowWeaponPage()
{
	if (WidgetSwitcher_1)
	{
		WidgetSwitcher_1->SetActiveWidgetIndex(WeaponPageIndex);
	}
}

UCharacterClassInfo* UClassSelectionWidget::GetSelectedClass() const
{
	return ObservedPlayerState ? ObservedPlayerState->GetCharacterClassInfo() : nullptr;
}

TSubclassOf<ANexusWeaponBase> UClassSelectionWidget::GetSelectedWeapon() const
{
	return ObservedPlayerState ? ObservedPlayerState->GetSelectedWeaponClass() : nullptr;
}

TArray<FNexusAbilityGrant> UClassSelectionWidget::GetSelectedClassAbilities() const
{
	return ObservedPlayerState
		? ObservedPlayerState->GetSelectedClassAbilityGrants()
		: TArray<FNexusAbilityGrant>();
}

bool UClassSelectionWidget::GetIsReadyLockedIn() const
{
	return ObservedPlayerState ? ObservedPlayerState->GetSelectionLockedIn() : false;
}

bool UClassSelectionWidget::CanClickReady() const
{
	return ObservedPlayerState ? ObservedPlayerState->CanLockInSelection() : false;
}

bool UClassSelectionWidget::IsClassSelected(UCharacterClassInfo* InClassInfo) const
{
	return GetSelectedClass() == InClassInfo;
}

bool UClassSelectionWidget::IsWeaponSelected(TSubclassOf<ANexusWeaponBase> InWeaponClass) const
{
	return GetSelectedWeapon() == InWeaponClass;
}

void UClassSelectionWidget::ClearClassEntryWidgets()
{
	for (UClassSelectionEntryWidget* Entry : CurrentClassEntries)
	{
		if (Entry)
		{
			Entry->OnClassChosen.RemoveDynamic(this, &ThisClass::HandleClassEntryChosen);
		}
	}

	CurrentClassEntries.Reset();

	if (ClassListBox)
	{
		ClassListBox->ClearChildren();
	}
}

void UClassSelectionWidget::ClearWeaponEntryWidgets()
{
	for (UWeaponSelectionEntryWidget* Entry : CurrentWeaponEntries)
	{
		if (Entry)
		{
			Entry->OnWeaponChosen.RemoveDynamic(this, &ThisClass::HandleWeaponEntryChosen);
		}
	}

	CurrentWeaponEntries.Reset();

	if (WeaponListBox)
	{
		WeaponListBox->ClearChildren();
	}
}

void UClassSelectionWidget::RebuildClassEntryWidgets()
{
	ClearClassEntryWidgets();

	if (!ClassListBox)
	{
		return;
	}

	ANexusGameState* GS = GetWorld() ? GetWorld()->GetGameState<ANexusGameState>() : nullptr;
	if (!GS)
	{
		GS = ObservedGameState;
	}

	if (!GS)
	{
		return;
	}

	TSubclassOf<UClassSelectionEntryWidget> EntryClass = ClassEntryWidgetClass;
	if (!EntryClass)
	{
		EntryClass = UClassSelectionEntryWidget::StaticClass();
	}

	for (UCharacterClassInfo* ClassInfo : GS->GetAvailableClasses())
	{
		if (!ClassInfo)
		{
			continue;
		}

		UClassSelectionEntryWidget* Entry = CreateWidget<UClassSelectionEntryWidget>(
			GetOwningPlayer(),
			EntryClass);

		if (!Entry)
		{
			continue;
		}

		Entry->InitializeClassEntry(ClassInfo);
		Entry->OnClassChosen.AddDynamic(this, &ThisClass::HandleClassEntryChosen);

		ClassListBox->AddChild(Entry);
		CurrentClassEntries.Add(Entry);
	}
}

void UClassSelectionWidget::RebuildWeaponEntryWidgets(UCharacterClassInfo* InClassInfo)
{
	ClearWeaponEntryWidgets();

	if (!WeaponListBox || !InClassInfo)
	{
		return;
	}

	TArray<TSubclassOf<ANexusWeaponBase>> WeaponOptions;
	ResolveWeaponOptionsForClass(InClassInfo, WeaponOptions);

	TSubclassOf<UWeaponSelectionEntryWidget> EntryClass = WeaponEntryWidgetClass;
	if (!EntryClass)
	{
		EntryClass = UWeaponSelectionEntryWidget::StaticClass();
	}

	for (const TSubclassOf<ANexusWeaponBase>& WeaponClass : WeaponOptions)
	{
		if (!WeaponClass)
		{
			continue;
		}

		UWeaponSelectionEntryWidget* Entry = CreateWidget<UWeaponSelectionEntryWidget>(
			GetOwningPlayer(),
			EntryClass);

		if (!Entry)
		{
			continue;
		}

		Entry->InitializeWeaponEntry(WeaponClass);
		Entry->OnWeaponChosen.AddDynamic(this, &ThisClass::HandleWeaponEntryChosen);

		WeaponListBox->AddChild(Entry);
		CurrentWeaponEntries.Add(Entry);
	}
}

void UClassSelectionWidget::RefreshClassEntryStates(
	UCharacterClassInfo* SelectedClass,
	bool bLockedIn)
{
	for (UClassSelectionEntryWidget* Entry : CurrentClassEntries)
	{
		if (!Entry)
		{
			continue;
		}

		Entry->SetEntrySelected(Entry->GetClassInfo() == SelectedClass);
		Entry->SetEntryInteractionEnabled(!bLockedIn);
	}
}

void UClassSelectionWidget::ResolveWeaponOptionsForClass(
	UCharacterClassInfo* InClassInfo,
	TArray<TSubclassOf<ANexusWeaponBase>>& OutWeaponClasses) const
{
	OutWeaponClasses.Reset();

	if (!InClassInfo)
	{
		return;
	}

	if (InClassInfo->AllowedWeaponClasses.Num() > 0)
	{
		for (const TSubclassOf<ANexusWeaponBase>& WeaponClass : InClassInfo->AllowedWeaponClasses)
		{
			if (WeaponClass)
			{
				OutWeaponClasses.AddUnique(WeaponClass);
			}
		}

		return;
	}

	const TSubclassOf<ANexusWeaponBase> DefaultWeaponClass = InClassInfo->GetResolvedDefaultWeaponClass();
	if (DefaultWeaponClass)
	{
		OutWeaponClasses.Add(DefaultWeaponClass);
	}
}

void UClassSelectionWidget::RefreshWeaponEntryStates(
	TSubclassOf<ANexusWeaponBase> SelectedWeaponClass,
	bool bLockedIn)
{
	for (UWeaponSelectionEntryWidget* Entry : CurrentWeaponEntries)
	{
		if (!Entry)
		{
			continue;
		}

		Entry->SetEntrySelected(Entry->GetWeaponClass() == SelectedWeaponClass);
		Entry->SetEntryInteractionEnabled(!bLockedIn);
	}
}

void UClassSelectionWidget::RefreshNativeControlStates(
	UCharacterClassInfo* SelectedClass,
	TSubclassOf<ANexusWeaponBase> SelectedWeaponClass,
	bool bLockedIn)
{
	const bool bCanGoBack = !bLockedIn && DisplayedClassInfo != nullptr;
	const bool bCanReadyNow = bLockedIn || CanClickReady();

	if (BackButton)
	{
		BackButton->SetIsEnabled(bCanGoBack);
	}

	if (ReadyButton)
	{
		ReadyButton->SetIsEnabled(bCanReadyNow);
	}

	if (SelectedClassText)
	{
		SelectedClassText->SetText(GetClassDisplayText(DisplayedClassInfo ? DisplayedClassInfo : SelectedClass));
	}

	RefreshClassEntryStates(SelectedClass, bLockedIn);
	RefreshWeaponEntryStates(SelectedWeaponClass, bLockedIn);
}

FText UClassSelectionWidget::GetClassDisplayText(UCharacterClassInfo* InClassInfo) const
{
	if (!InClassInfo)
	{
		return FText::GetEmpty();
	}

	FString AssetName = InClassInfo->GetName();
	AssetName.RemoveFromStart(TEXT("DA_"));

	return FText::FromString(AssetName);
}
