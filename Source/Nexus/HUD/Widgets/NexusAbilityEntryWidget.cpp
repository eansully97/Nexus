#include "Nexus/HUD/Widgets/NexusAbilityEntryWidget.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Nexus/Character/NexusCharacterBase.h"
#include "Nexus/Controller/NexusPlayerController.h"
#include "Nexus/DataAssets/AbilityInfo.h"
#include "Nexus/FunctionLibraries/NexusAbilityFunctionLibrary.h"
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
	ObservedPlayerController = nullptr;
	bHasValidTargetForObservedAbility = true;

	if (const APawn* ObservedPawn = Cast<APawn>(ObservedActor))
	{
		ObservedPlayerController = Cast<ANexusPlayerController>(ObservedPawn->GetController());
	}

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

		const FDelegateHandle Handle = TagEvent.AddUObject(
			this,
			&UNexusAbilityEntryWidget::HandleCooldownGameplayTagChanged);

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

void UNexusAbilityEntryWidget::BindTargetingListener()
{
	UnbindTargetingListener();

	if (!ObservedPlayerController || !ObservedAbility || !DoesAbilityNeedValidTarget())
	{
		return;
	}

	ObservedPlayerController->OnControllerTargetChanged.AddDynamic(
		this,
		&ThisClass::HandleControllerTargetChanged);
}

void UNexusAbilityEntryWidget::UnbindTargetingListener()
{
	if (!ObservedPlayerController)
	{
		return;
	}

	ObservedPlayerController->OnControllerTargetChanged.RemoveDynamic(
		this,
		&ThisClass::HandleControllerTargetChanged);
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
	if (!ObservedAbility)
	{
		return;
	}

	BindCooldownListeners();
	BindTargetingListener();

	UpdateAbilityActiveState();
	UpdateTargetRequirementState();

	if (UWorld* World = GetWorld())
	{
		if (!World->GetTimerManager().IsTimerActive(ActiveStateTimerHandle))
		{
			World->GetTimerManager().SetTimer(
				ActiveStateTimerHandle,
				this,
				&UNexusAbilityEntryWidget::UpdateAbilityActiveState,
				ActiveStateRefreshRate,
				true);
		}
	}
}

void UNexusAbilityEntryWidget::UnbindListeners()
{
	UnbindCooldownListeners();
	UnbindTargetingListener();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ActiveStateTimerHandle);
	}

	bHasValidTargetForObservedAbility = true;
	ObservedPlayerController = nullptr;

	if (TargetRequiredOverlay)
	{
		TargetRequiredOverlay->SetVisibility(ESlateVisibility::Hidden);
	}
}

void UNexusAbilityEntryWidget::HandleCooldownGameplayTagChanged(const FGameplayTag GameplayTag, int32 NewCount)
{
	UpdateCooldownProgress();
}

void UNexusAbilityEntryWidget::HandleControllerTargetChanged(ANexusCharacterBase* NewTarget, bool bHasValidTarget)
{
	UpdateTargetRequirementState();
}

void UNexusAbilityEntryWidget::RefreshFromAbility()
{
	if (!ObservedASC || !AbilitySpecHandle.IsValid())
	{
		ObservedAbility = nullptr;
		ResetCooldownVisuals();
		UpdateAbilityBGColor(false);

		if (TargetRequiredOverlay)
		{
			TargetRequiredOverlay->SetVisibility(ESlateVisibility::Hidden);
		}
		return;
	}

	bool bIsInstance = false;
	const UGameplayAbility* BaseAbility =
		UAbilitySystemBlueprintLibrary::GetGameplayAbilityFromSpecHandle(
			ObservedASC,
			AbilitySpecHandle,
			bIsInstance);

	ObservedAbility = Cast<UNexusGameplayAbility>(BaseAbility);

	if (!ObservedAbility)
	{
		ResetCooldownVisuals();
		UpdateAbilityBGColor(false);

		if (TargetRequiredOverlay)
		{
			TargetRequiredOverlay->SetVisibility(ESlateVisibility::Hidden);
		}
		return;
	}

	if (AbilityIcon)
	{
		if (ObservedAbility->AbilityInfo && ObservedAbility->AbilityInfo->AbilityIcon)
		{
			AbilityIcon->SetBrushFromTexture(ObservedAbility->AbilityInfo->AbilityIcon);
		}
		else
		{
			AbilityIcon->SetBrushFromTexture(nullptr);
		}
	}

	ResetCooldownVisuals();
	UpdateCooldownProgress();
	UpdateAbilityActiveState();
	UpdateTargetRequirementState();

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
		if (!IsObservedAbilityActive())
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
					true);
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

bool UNexusAbilityEntryWidget::DoesAbilityNeedValidTarget() const
{
	return IsValid(ObservedAbility) && ObservedAbility->bRequiresValidTarget;
}

bool UNexusAbilityEntryWidget::IsLocalTargetUsableForObservedAbility() const
{
	if (!ObservedAbility)
	{
		return false;
	}

	if (!DoesAbilityNeedValidTarget())
	{
		return true;
	}

	const APawn* ObservedPawn = Cast<APawn>(ObservedActor);
	if (!IsValid(ObservedPawn))
	{
		return false;
	}

	const ANexusPlayerController* PC = Cast<ANexusPlayerController>(ObservedPawn->GetController());
	const ANexusCharacterBase* SourceCharacter = Cast<ANexusCharacterBase>(ObservedPawn);

	if (!IsValid(PC) || !IsValid(SourceCharacter))
	{
		return false;
	}

	ANexusCharacterBase* UsableTarget = nullptr;
	return UNexusAbilityFunctionLibrary::TryGetUsableControllerTargetForAbility(
		PC,
		SourceCharacter,
		ObservedAbility,
		UsableTarget);
}

void UNexusAbilityEntryWidget::UpdateTargetRequirementState()
{
	bool bHasValidTargetForAbility = true;

	if (ObservedAbility)
	{
		bHasValidTargetForAbility = IsLocalTargetUsableForObservedAbility();
	}

	bHasValidTargetForObservedAbility = bHasValidTargetForAbility;

	if (TargetRequiredOverlay && (!CooldownOverlay || !CooldownOverlay->IsVisible()))
	{
		TargetRequiredOverlay->SetVisibility(
			bHasValidTargetForAbility
				? ESlateVisibility::Hidden
				: ESlateVisibility::Visible);
	}

	BP_OnTargetRequirementChanged(bHasValidTargetForAbility);
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
