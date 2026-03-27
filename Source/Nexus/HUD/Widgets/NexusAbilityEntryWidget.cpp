#include "NexusAbilityEntryWidget.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "Nexus/DataAssets/DataAsset_AbilityInfo.h"
#include "Nexus/GameplayAbilitySystem/Abilities/NexusGameplayAbility.h"

void UNexusAbilityEntryWidget::NativeDestruct()
{
	UnbindListeners();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CooldownTimerHandle);
		World->GetTimerManager().ClearTimer(ActiveStateTimerHandle);
	}

	Super::NativeDestruct();
}

void UNexusAbilityEntryWidget::InitializeAbilityEntry(
	AActor* InObservedActor,
	UAbilitySystemComponent* InASC,
	FGameplayAbilitySpecHandle InSpecHandle)
{
	UnbindListeners();

	ObservedActor = InObservedActor;
	ObservedASC = InASC;
	AbilitySpecHandle = InSpecHandle;
	ObservedAbility = nullptr;
	ObservedCooldownTags.Reset();

	RefreshFromAbility();
	BindListeners();
}

void UNexusAbilityEntryWidget::BindCooldownListeners()
{
	UnbindCooldownListeners();

	if (!ObservedASC || !ObservedAbility)
	{
		return;
	}

	const FGameplayTagContainer* CooldownTags = ObservedAbility->GetCooldownTags();
	if (!CooldownTags || CooldownTags->Num() == 0)
	{
		return;
	}

	ObservedCooldownTags = *CooldownTags;

	for (const FGameplayTag& CooldownTag : ObservedCooldownTags)
	{
		FOnGameplayEffectTagCountChanged& TagEvent =
			ObservedASC->RegisterGameplayTagEvent(CooldownTag, EGameplayTagEventType::NewOrRemoved);

		FDelegateHandle Handle = TagEvent.AddUObject(
			this,
			&UNexusAbilityEntryWidget::HandleCooldownGameplayTagChanged
		);

		CooldownTagEventHandles.Add(Handle);
	}
}

void UNexusAbilityEntryWidget::UnbindCooldownListeners()
{
	if (!ObservedASC || CooldownTagEventHandles.Num() == 0 || ObservedCooldownTags.Num() == 0)
	{
		CooldownTagEventHandles.Empty();
		ObservedCooldownTags.Reset();
		return;
	}

	int32 Index = 0;
	for (const FGameplayTag& CooldownTag : ObservedCooldownTags)
	{
		if (CooldownTagEventHandles.IsValidIndex(Index))
		{
			ObservedASC->RegisterGameplayTagEvent(CooldownTag, EGameplayTagEventType::NewOrRemoved)
				.Remove(CooldownTagEventHandles[Index]);
		}
		++Index;
	}

	CooldownTagEventHandles.Empty();
	ObservedCooldownTags.Reset();
}

void UNexusAbilityEntryWidget::UpdateAbilityBGColor(bool bIsActive) const
{
	if (!AbilityBG)
	{
		return;
	}

	AbilityBG->SetColorAndOpacity(bIsActive ? FLinearColor::Yellow : FLinearColor::White);
}

void UNexusAbilityEntryWidget::BindListeners()
{
	BindCooldownListeners();
	UpdateAbilityActiveState();

	if (UWorld* World = GetWorld())
	{
		if (!World->GetTimerManager().IsTimerActive(ActiveStateTimerHandle))
		{
			World->GetTimerManager().SetTimer(
				ActiveStateTimerHandle,
				this,
				&UNexusAbilityEntryWidget::UpdateAbilityActiveState,
				ActiveStateRefreshRate,
				true
			);
		}
	}
}

void UNexusAbilityEntryWidget::UnbindListeners()
{
	UnbindCooldownListeners();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ActiveStateTimerHandle);
	}
}


void UNexusAbilityEntryWidget::HandleCooldownGameplayTagChanged(const FGameplayTag GameplayTag, int32 NewCount)
{
	UpdateCooldownProgress();
}


void UNexusAbilityEntryWidget::RefreshFromAbility()
{
	if (!ObservedASC || !AbilitySpecHandle.IsValid())
	{
		ResetCooldownVisuals();
		return;
	}

	bool bIsInstance = false;
	const UGameplayAbility* BaseAbility =
		UAbilitySystemBlueprintLibrary::GetGameplayAbilityFromSpecHandle(ObservedASC, AbilitySpecHandle, bIsInstance);

	ObservedAbility = Cast<UNexusGameplayAbility>(BaseAbility);

	if (!ObservedAbility)
	{
		ResetCooldownVisuals();
		return;
	}

	if (AbilityIcon)
	{
		AbilityIcon->SetBrushFromTexture(ObservedAbility->AbilityInfo->AbilityIcon);
	}

	ResetCooldownVisuals();
	UpdateCooldownProgress();
	UpdateAbilityActiveState();

	BP_OnAbilityInitialized();
}

void UNexusAbilityEntryWidget::UpdateCooldownProgress()
{
	if (!ObservedAbility)
	{
		return;
	}

	const float TimeRemaining = ObservedAbility->GetCooldownTimeRemaining();
	if (TimeRemaining > 0.f)
	{
		if (CooldownText)
		{
			CooldownText->SetVisibility(ESlateVisibility::Visible);
			CooldownText->SetText(FText::AsNumber(FMath::CeilToInt(TimeRemaining)));
		}

		if (CooldownOverlay)
		{
			CooldownOverlay->SetVisibility(ESlateVisibility::Visible);
		}

		BP_UpdateCooldownVisuals(TimeRemaining);

		if (UWorld* World = GetWorld())
		{
			if (!World->GetTimerManager().IsTimerActive(CooldownTimerHandle))
			{
				World->GetTimerManager().SetTimer(
					CooldownTimerHandle,
					this,
					&UNexusAbilityEntryWidget::UpdateCooldownProgress,
					CooldownRefreshRate,
					true
				);
			}
		}
	}
	else
	{
		ResetCooldownVisuals();

		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(CooldownTimerHandle);
		}

		BP_OnCooldownFinished();
	}
}

bool UNexusAbilityEntryWidget::IsObservedAbilityActive() const
{
	if (!ObservedASC || !AbilitySpecHandle.IsValid())
	{
		return false;
	}

	const FGameplayAbilitySpec* AbilitySpec = ObservedASC->FindAbilitySpecFromHandle(AbilitySpecHandle);
	if (!AbilitySpec)
	{
		return false;
	}
	
	return AbilitySpec->IsActive();
}

void UNexusAbilityEntryWidget::UpdateAbilityActiveState()
{
	UpdateAbilityBGColor(IsObservedAbilityActive());
}

void UNexusAbilityEntryWidget::ResetCooldownVisuals()
{
	if (CooldownText)
	{
		CooldownText->SetText(FText::GetEmpty());
		CooldownText->SetVisibility(ESlateVisibility::Hidden);
	}

	if (CooldownOverlay)
	{
		CooldownOverlay->SetVisibility(ESlateVisibility::Hidden);
	}
}