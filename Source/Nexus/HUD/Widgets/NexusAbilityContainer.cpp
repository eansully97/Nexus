#include "NexusAbilityContainer.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "NexusAbilityEntryWidget.h"
#include "Abilities/Async/AbilityAsync_WaitGameplayEvent.h"
#include "Abilities/Async/AbilityAsync_WaitGameplayTagCountChanged.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Pawn.h"
#include "Nexus/Character/NexusCharacterBase.h"
#include "Nexus/GameplayAbilitySystem/Abilities/NexusGameplayAbility.h"

void UNexusAbilityContainer::NativeConstruct()
{
	Super::NativeConstruct();

	if (APawn* OwningPawn = GetOwningPlayerPawn())
	{
		SetObservedPawn(OwningPawn);
	}
	else
	{
		RefreshContainer();
	}
}

void UNexusAbilityContainer::NativeDestruct()
{
	UnbindAbilitiesChangedEvent();
	Super::NativeDestruct();
}

void UNexusAbilityContainer::SetObservedPawn(APawn* NewPawn)
{
	ANexusCharacterBase* NewCharacter = Cast<ANexusCharacterBase>(NewPawn);

	if (ObservedCharacter == NewCharacter)
	{
		return;
	}

	UnbindAbilitiesChangedEvent();

	ObservedCharacter = NewCharacter;
	ObservedASC = nullptr;

	if (ObservedCharacter)
	{
		ObservedASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(ObservedCharacter);
		BindAbilitiesChangedEvent();
	}

	RefreshContainer();
}

void UNexusAbilityContainer::BindAbilitiesChangedEvent()
{
	UnbindAbilitiesChangedEvent();

	if (!ObservedCharacter)
	{
		return;
	}

	ObservedCharacter->OnAbilitiesChanged.AddDynamic(
		this,
		&UNexusAbilityContainer::HandleAbilitiesChanged
	);
}

void UNexusAbilityContainer::UnbindAbilitiesChangedEvent()
{
	if (!ObservedCharacter)
	{
		return;
	}

	ObservedCharacter->OnAbilitiesChanged.RemoveDynamic(
		this,
		&UNexusAbilityContainer::HandleAbilitiesChanged
	);
}

void UNexusAbilityContainer::HandleAbilitiesChanged()
{
	RefreshContainer();
}

void UNexusAbilityContainer::RefreshContainer()
{
	GetAbilitiesForContainer();
	RebuildContainer();
}

void UNexusAbilityContainer::GetAbilitiesForContainer()
{
	AbilitiesForContainer.Empty();

	if (!ObservedASC)
	{
		return;
	}

	TArray<FGameplayAbilitySpecHandle> AllAbilityHandles;
	ObservedASC->GetAllAbilities(AllAbilityHandles);

	for (const FGameplayAbilitySpecHandle& SpecHandle : AllAbilityHandles)
	{
		bool bIsInstance = false;
		const UGameplayAbility* BaseAbility =
			UAbilitySystemBlueprintLibrary::GetGameplayAbilityFromSpecHandle(ObservedASC, SpecHandle, bIsInstance);

		const UNexusGameplayAbility* NexusAbility = Cast<UNexusGameplayAbility>(BaseAbility);
		if (!NexusAbility)
		{
			continue;
		}

		if (NexusAbility->GetContainerToShowIn() == ContainerType)
		{
			AbilitiesForContainer.Add(SpecHandle);
		}
	}
}

void UNexusAbilityContainer::RebuildContainer()
{
	if (!AbilityContainer)
	{
		return;
	}

	AbilityContainer->ClearChildren();

	if (!AbilityEntryWidgetClass || !ObservedASC || !GetOwningPlayer())
	{
		return;
	}

	for (const FGameplayAbilitySpecHandle& SpecHandle : AbilitiesForContainer)
	{
		UNexusAbilityEntryWidget* EntryWidget =
			CreateWidget<UNexusAbilityEntryWidget>(GetOwningPlayer(), AbilityEntryWidgetClass);

		if (!EntryWidget)
		{
			continue;
		}

		EntryWidget->InitializeAbilityEntry(ObservedCharacter,ObservedASC, SpecHandle);

		if (UHorizontalBoxSlot* Container = AbilityContainer->AddChildToHorizontalBox(EntryWidget))
		{
			Container->SetPadding(FMargin(5.f));
		}
	}
}