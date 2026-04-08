#include "NexusAbilityContainer.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Nexus/Character/NexusCharacterBase.h"
#include "Nexus/GameplayAbilitySystem/Abilities/NexusGameplayAbility.h"
#include "Nexus/HUD/Widgets/NexusAbilityEntryWidget.h"

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

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DeferredRefreshTimerHandle);
	}

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
	LastBuiltAbilityHandles.Reset();

	if (ObservedCharacter)
	{
		ObservedASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(ObservedCharacter);
		BindAbilitiesChangedEvent();
	}

	RefreshContainer();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DeferredRefreshTimerHandle);
		World->GetTimerManager().SetTimer(
			DeferredRefreshTimerHandle,
			this,
			&ThisClass::HandleDeferredRefresh,
			0.05f,
			false);
	}
}

void UNexusAbilityContainer::BindAbilitiesChangedEvent()
{
	UnbindAbilitiesChangedEvent();

	if (!ObservedCharacter)
	{
		return;
	}

	ObservedCharacter->OnGrantedAbilitiesChanged.AddDynamic(
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

	ObservedCharacter->OnGrantedAbilitiesChanged.RemoveDynamic(
		this,
		&UNexusAbilityContainer::HandleAbilitiesChanged
	);
}

void UNexusAbilityContainer::HandleAbilitiesChanged()
{
	RefreshContainer();
}

void UNexusAbilityContainer::HandleDeferredRefresh()
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
			UAbilitySystemBlueprintLibrary::GetGameplayAbilityFromSpecHandle(
				ObservedASC,
				SpecHandle,
				bIsInstance);

		const UNexusGameplayAbility* NexusAbility = Cast<UNexusGameplayAbility>(BaseAbility);
		if (!NexusAbility)
		{
			continue;
		}

		if (AbilitiesForContainer.Contains(SpecHandle))
		{
			continue;
		}

		if (NexusAbility->GetContainerToShowIn() == ContainerType)
		{
			AbilitiesForContainer.AddUnique(SpecHandle);
		}
	}
}

bool UNexusAbilityContainer::HasSameAbilitySet(const TArray<FGameplayAbilitySpecHandle>& NewHandles) const
{
	if (LastBuiltAbilityHandles.Num() != NewHandles.Num())
	{
		return false;
	}

	for (int32 Index = 0; Index < NewHandles.Num(); ++Index)
	{
		if (LastBuiltAbilityHandles[Index] != NewHandles[Index])
		{
			return false;
		}
	}

	return true;
}

void UNexusAbilityContainer::RebuildContainer()
{
	if (!AbilityContainer)
	{
		return;
	}

	if (!ObservedASC)
	{
		AbilityContainer->ClearChildren();
		LastBuiltAbilityHandles.Reset();
		return;
	}

	if (HasSameAbilitySet(AbilitiesForContainer))
	{
		return;
	}

	AbilityContainer->ClearChildren();

	if (!AbilityEntryWidgetClass || !GetOwningPlayer())
	{
		LastBuiltAbilityHandles.Reset();
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

		EntryWidget->InitializeAbilityEntry(ObservedCharacter, ObservedASC, SpecHandle);

		if (UHorizontalBoxSlot* ContainerSlot = AbilityContainer->AddChildToHorizontalBox(EntryWidget))
		{
			ContainerSlot->SetPadding(FMargin(5.f));
		}
	}

	LastBuiltAbilityHandles = AbilitiesForContainer;
}