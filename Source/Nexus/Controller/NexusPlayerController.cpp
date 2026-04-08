#include "Nexus/Controller/NexusPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "Camera/CameraActor.h"
#include "Engine/Engine.h"
#include "EnhancedInputSubsystems.h"
#include "Kismet/GameplayStatics.h"
#include "Widgets/SWidget.h"
#include "Nexus/Character/NexusCharacterBase.h"
#include "Nexus/Character/Player/NexusPlayerCharacter.h"
#include "Nexus/FunctionLibraries/NexusAbilityFunctionLibrary.h"
#include "Nexus/FunctionLibraries/NexusCombatFunctionLibrary.h"
#include "Nexus/GameInstance/NexusGameInstance.h"
#include "Nexus/GameMode/NexusGameMode.h"
#include "Nexus/GameState/NexusGameState.h"
#include "Nexus/HUD/NexusHUD.h"
#include "Nexus/HUD/Widgets/ClassSelectionWidget.h"
#include "Nexus/HUD/Widgets/NexusMainHUDWidget.h"
#include "Nexus/PlayerState/NexusPlayerState.h"
#include "Nexus/Weapons/NexusWeaponBase.h"

DEFINE_LOG_CATEGORY_STATIC(LogNexusPlayerController, Log, All);

ANexusPlayerController::ANexusPlayerController()
{
	bAutoManageActiveCameraTarget = false;
}

ACameraActor* ANexusPlayerController::GetWaitingCameraActor(ENexusTeamID InTeamID) const
{
	FName DesiredTag = NAME_None;

	switch (InTeamID)
	{
	case ENexusTeamID::TeamA:
		DesiredTag = FName("TeamACam");
		break;

	case ENexusTeamID::TeamB:
		DesiredTag = FName("TeamBCam");
		break;

	default:
		return nullptr;
	}

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), DesiredTag, FoundActors);

	for (AActor* Actor : FoundActors)
	{
		if (ACameraActor* CameraActor = Cast<ACameraActor>(Actor))
		{
			return CameraActor;
		}
	}

	UE_LOG(LogNexusPlayerController, Warning, TEXT("No waiting camera actor found with tag: %s"), *DesiredTag.ToString());
	return nullptr;
}

void ANexusPlayerController::BindToObservedGameState()
{
	ANexusGameState* NewGameState = GetWorld() ? GetWorld()->GetGameState<ANexusGameState>() : nullptr;
	if (ObservedGameState == NewGameState && ObservedGameState)
	{
		return;
	}

	UnbindFromObservedGameState();
	ObservedGameState = NewGameState;

	if (ObservedGameState)
	{
		ObservedGameState->OnClassSelectOpenChanged.AddDynamic(
			this,
			&ThisClass::HandleObservedClassSelectOpenChanged);
		ObservedGameState->OnMatchEndedChanged.AddDynamic(
			this,
			&ThisClass::HandleObservedMatchEndedChanged);
	}
}

void ANexusPlayerController::UnbindFromObservedGameState()
{
	if (ObservedGameState)
	{
		ObservedGameState->OnClassSelectOpenChanged.RemoveDynamic(
			this,
			&ThisClass::HandleObservedClassSelectOpenChanged);
		ObservedGameState->OnMatchEndedChanged.RemoveDynamic(
			this,
			&ThisClass::HandleObservedMatchEndedChanged);
	}

	ObservedGameState = nullptr;
}

void ANexusPlayerController::HandleObservedClassSelectOpenChanged(bool bIsOpen)
{
	HandleClassSelectStateChanged();
}

void ANexusPlayerController::HandleObservedMatchEndedChanged(bool bHasEnded, ENexusTeamID WinningTeam)
{
	if (!bHasEnded || !IsLocalController())
	{
		return;
	}

	HidePauseMenu();
	HideClassSelectUI();
	ShowWaitingCameraForTeam();
	ShowMatchEndedMessage(WinningTeam);
}

void ANexusPlayerController::ShowMatchEndedMessage(ENexusTeamID WinningTeam)
{
	const FString ResultText = [&]()
	{
		if (WinningTeam == ENexusTeamID::Neutral)
		{
			return FString(TEXT("Match ended in a draw."));
		}

		return GetTeamID() == WinningTeam
			? FString(TEXT("Victory!"))
			: FString(TEXT("Defeat."));
	}();

	const ANexusGameState* GS = GetWorld() ? GetWorld()->GetGameState<ANexusGameState>() : nullptr;
	const int32 CountdownSeconds = GS ? GS->PostMatchCountdownSeconds : 0;
	const FString Message = CountdownSeconds > 0
		? FString::Printf(TEXT("%s Returning to the lobby in %d seconds."), *ResultText, CountdownSeconds)
		: ResultText;

	if (GEngine)
	{
		const FColor MessageColor = WinningTeam == ENexusTeamID::Neutral
			? FColor::Silver
			: (GetTeamID() == WinningTeam ? FColor::Green : FColor::Red);

		GEngine->AddOnScreenDebugMessage(
			reinterpret_cast<uint64>(this),
			FMath::Max(5.f, static_cast<float>(CountdownSeconds)),
			MessageColor,
			Message);
	}

	ClientMessage(Message);
}

ENexusTeamID ANexusPlayerController::GetTeamID() const
{
	const ANexusPlayerState* PS = GetPlayerState<ANexusPlayerState>();
	return PS ? PS->GetTeamID() : ENexusTeamID::Neutral;
}

void ANexusPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
		{
			Subsystem->AddMappingContext(CurrentContext, 0);
		}
	}
	BindToObservedGameState();
	HandleClassSelectStateChanged();
}

void ANexusPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	RefreshHUDBindings();

	CurrentTargetedCharacter = nullptr;
	bHasValidTarget = false;
	OnControllerTargetChanged.Broadcast(nullptr, false);
}

void ANexusPlayerController::OnRep_Pawn()
{
	Super::OnRep_Pawn();
	RefreshHUDBindings();

	CurrentTargetedCharacter = nullptr;
	bHasValidTarget = false;
	OnControllerTargetChanged.Broadcast(nullptr, false);
}

void ANexusPlayerController::PawnLeavingGame()
{
	APawn* ControlledPawn = GetPawn();
	ANexusGameMode* NexusGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ANexusGameMode>() : nullptr;
	const ANexusCharacterBase* NexusCharacter = Cast<ANexusCharacterBase>(ControlledPawn);

	if (ControlledPawn && NexusGameMode && NexusGameMode->ShouldPreserveDisconnectedPawn(this, NexusCharacter))
	{
		NexusGameMode->CacheReconnectReservation(this);
		UnPossess();
		return;
	}

	Super::PawnLeavingGame();
}

void ANexusPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!IsLocalController())
	{
		return;
	}

	GetCrosshairHitResult(CurrentCrosshairHit, 10000.f);
	UpdateTargetedCharacter();
}

void ANexusPlayerController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	
	BindToObservedGameState();
	HandleClassSelectStateChanged();
	BindClassSelectWidgetToPlayerState();

	if (ClassSelectWidget)
	{
		ClassSelectWidget->RefreshFromObservedState();
	}
}

void ANexusPlayerController::RefreshHUDBindings()
{
	if (!IsLocalController())
	{
		return;
	}

	ANexusHUD* NexusHUD = Cast<ANexusHUD>(GetHUD());
	if (!NexusHUD)
	{
		return;
	}

	if (!NexusHUD->GetMainHUDWidget())
	{
		NexusHUD->InitMainHUDWidget();
	}

	UNexusMainHUDWidget* MainWidget = NexusHUD->GetMainHUDWidget();
	if (!MainWidget)
	{
		return;
	}

	MainWidget->SetObservedPawn(GetPawn());
}

bool ANexusPlayerController::GetCrosshairHitResult(FHitResult& OutHit, float TraceDistance) const
{
	OutHit = FHitResult();

	int32 ViewportX = 0;
	int32 ViewportY = 0;
	GetViewportSize(ViewportX, ViewportY);

	const FVector2D ScreenCenter(
		ViewportX * 0.5f,
		ViewportY * 0.5f
	);

	FVector WorldLocation = FVector::ZeroVector;
	FVector WorldDirection = FVector::ForwardVector;

	if (!DeprojectScreenPositionToWorld(
		ScreenCenter.X,
		ScreenCenter.Y,
		WorldLocation,
		WorldDirection))
	{
		return false;
	}

	const FVector TraceStart = WorldLocation;
	const FVector TraceEnd = TraceStart + (WorldDirection * TraceDistance);

	FCollisionQueryParams Params;
	Params.bTraceComplex = true;
	Params.AddIgnoredActor(GetPawn());

	return GetWorld()->LineTraceSingleByChannel(
		OutHit,
		TraceStart,
		TraceEnd,
		ECC_Visibility,
		Params
	);
}

FHitResult ANexusPlayerController::GetCurrentCrosshairHit() const
{
	return CurrentCrosshairHit;
}

void ANexusPlayerController::BroadcastTargetChangedIfNeeded(
	ANexusCharacterBase* PreviousTarget,
	bool bPreviousHasValidTarget)
{
	if (PreviousTarget != CurrentTargetedCharacter || bPreviousHasValidTarget != bHasValidTarget)
	{
		OnControllerTargetChanged.Broadcast(CurrentTargetedCharacter, bHasValidTarget);
	}
}

void ANexusPlayerController::UpdateTargetedCharacter()
{
	ANexusCharacterBase* PreviousTarget = CurrentTargetedCharacter;
	const bool bPreviousHasValidTarget = bHasValidTarget;

	CurrentTargetedCharacter = nullptr;
	bHasValidTarget = false;

	ANexusPlayerCharacter* SourceCharacter = Cast<ANexusPlayerCharacter>(GetPawn());
	if (!IsValid(SourceCharacter))
	{
		BroadcastTargetChangedIfNeeded(PreviousTarget, bPreviousHasValidTarget);
		return;
	}

	ANexusCharacterBase* TargetCharacter =
		UNexusCombatFunctionLibrary::GetNexusCharacterFromActor(CurrentCrosshairHit.GetActor());

	if (!IsValidTargetCharacter(SourceCharacter, TargetCharacter))
	{
		BroadcastTargetChangedIfNeeded(PreviousTarget, bPreviousHasValidTarget);
		return;
	}

	CurrentTargetedCharacter = TargetCharacter;
	bHasValidTarget = true;

	BroadcastTargetChangedIfNeeded(PreviousTarget, bPreviousHasValidTarget);
}

bool ANexusPlayerController::IsValidTargetCharacter(
	ANexusCharacterBase* SourceCharacter,
	ANexusCharacterBase* TargetCharacter) const
{
	return UNexusCombatFunctionLibrary::IsValidLivingEnemyTarget(SourceCharacter, TargetCharacter);
}

bool ANexusPlayerController::HasUsableTargetForAbility(const UNexusGameplayAbility* Ability) const
{
	ANexusCharacterBase* OutTarget = nullptr;
	const ANexusCharacterBase* SourceCharacter = Cast<ANexusCharacterBase>(GetPawn());

	return UNexusAbilityFunctionLibrary::TryGetUsableControllerTargetForAbility(
		this,
		SourceCharacter,
		Ability,
		OutTarget);
}

ANexusCharacterBase* ANexusPlayerController::GetUsableTargetForAbility(const UNexusGameplayAbility* Ability) const
{
	ANexusCharacterBase* OutTarget = nullptr;
	const ANexusCharacterBase* SourceCharacter = Cast<ANexusCharacterBase>(GetPawn());

	UNexusAbilityFunctionLibrary::TryGetUsableControllerTargetForAbility(
		this,
		SourceCharacter,
		Ability,
		OutTarget);

	return OutTarget;
}

void ANexusPlayerController::ShowWaitingCameraForTeam()
{
	if (!IsLocalController())
	{
		return;
	}

	if (ACameraActor* WaitingCamera = GetWaitingCameraActor(GetTeamID()))
	{
		SetViewTargetWithBlend(WaitingCamera, 0.0f);
	}
}

void ANexusPlayerController::ClientRefreshClassSelectState_Implementation()
{
	HandleClassSelectStateChanged();
	BindClassSelectWidgetToPlayerState();

	if (ClassSelectWidget)
	{
		ClassSelectWidget->RefreshFromObservedState();
	}
}

void ANexusPlayerController::ClientCacheReconnectToken_Implementation(const FString& ReconnectToken)
{
	if (ReconnectToken.IsEmpty())
	{
		return;
	}

	if (UNexusGameInstance* GameInstance = GetGameInstance<UNexusGameInstance>())
	{
		GameInstance->CacheLocalReconnectToken(ReconnectToken);
	}
}

void ANexusPlayerController::ReturnToPawnCamera()
{
	if (!IsLocalController())
	{
		return;
	}

	if (APawn* ControlledPawn = GetPawn())
	{
		SetViewTargetWithBlend(ControlledPawn, 0.0f);
	}
}

void ANexusPlayerController::BindClassSelectWidgetToPlayerState()
{
	if (ClassSelectWidget)
	{
		ClassSelectWidget->BindToObservedPlayerState();
	}
}

void ANexusPlayerController::ShowClassSelectUI()
{
	if (!IsLocalController() || !ClassSelectWidgetClass || ClassSelectWidget)
	{
		return;
	}

	ClassSelectWidget = CreateWidget<UClassSelectionWidget>(this, ClassSelectWidgetClass);
	if (ClassSelectWidget)
	{
		ClassSelectWidget->AddToViewport();
		BindClassSelectWidgetToPlayerState();
		ClassSelectWidget->RefreshFromObservedState();
		ApplyMenuInputMode(ClassSelectWidget);
	}
}

void ANexusPlayerController::HideClassSelectUI()
{
	if (!IsLocalController())
	{
		return;
	}

	if (ClassSelectWidget)
	{
		ClassSelectWidget->RemoveFromParent();
		ClassSelectWidget = nullptr;
	}

	if (PauseMenuWidget)
	{
		ApplyMenuInputMode(PauseMenuWidget);
	}
	else
	{
		RestoreGameplayInputMode();
	}
}

void ANexusPlayerController::TogglePauseMenu()
{
	if (!IsLocalController())
	{
		return;
	}

	if (ObservedGameState && ObservedGameState->bClassSelectOpen)
	{
		return;
	}

	if (PauseMenuWidget)
	{
		HidePauseMenu();
	}
	else
	{
		ShowPauseMenu();
	}
}

void ANexusPlayerController::ShowPauseMenu()
{
	if (!IsLocalController() || !PauseMenuWidgetClass || PauseMenuWidget)
	{
		return;
	}

	if (ObservedGameState && ObservedGameState->bClassSelectOpen)
	{
		return;
	}

	PauseMenuWidget = CreateWidget<UUserWidget>(this, PauseMenuWidgetClass);
	if (!PauseMenuWidget)
	{
		return;
	}

	PauseMenuWidget->AddToViewport(50);
	ApplyMenuInputMode(PauseMenuWidget);
}

void ANexusPlayerController::HidePauseMenu()
{
	if (!IsLocalController())
	{
		return;
	}

	if (PauseMenuWidget)
	{
		PauseMenuWidget->RemoveFromParent();
		PauseMenuWidget = nullptr;
	}

	if (ClassSelectWidget)
	{
		ApplyMenuInputMode(ClassSelectWidget);
	}
	else
	{
		RestoreGameplayInputMode();
	}
}

void ANexusPlayerController::Server_SelectClass_Implementation(UCharacterClassInfo* InClassInfo)
{
	ANexusPlayerState* PS = GetPlayerState<ANexusPlayerState>();
	ANexusGameMode* GM = GetWorld()->GetAuthGameMode<ANexusGameMode>();

	if (!PS || !GM || !GM->IsClassSelectionOpen())
	{
		return;
	}

	if (!GM->IsValidClassChoice(InClassInfo))
	{
		return;
	}

	PS->SetCharacterClassInfo(InClassInfo);
	GM->RefreshReadyStatus();
}

void ANexusPlayerController::Server_SelectWeapon_Implementation(TSubclassOf<ANexusWeaponBase> InWeaponClass)
{
	ANexusPlayerState* PS = GetPlayerState<ANexusPlayerState>();
	ANexusGameMode* GM = GetWorld()->GetAuthGameMode<ANexusGameMode>();

	if (!PS || !GM || !GM->IsClassSelectionOpen())
	{
		return;
	}

	if (!PS->GetCharacterClassInfo())
	{
		return;
	}

	if (!PS->IsWeaponAllowedForCurrentClass(InWeaponClass))
	{
		return;
	}

	PS->SetSelectedWeaponClass(InWeaponClass);
	GM->RefreshReadyStatus();
}

void ANexusPlayerController::Server_SetSelectedClassAbilities_Implementation(
	const TArray<FNexusAbilityGrant>& InAbilityGrants)
{
	ANexusPlayerState* PS = GetPlayerState<ANexusPlayerState>();
	ANexusGameMode* GM = GetWorld()->GetAuthGameMode<ANexusGameMode>();

	if (!PS || !GM || !GM->IsClassSelectionOpen())
	{
		return;
	}

	if (!PS->GetCharacterClassInfo())
	{
		return;
	}

	PS->SetSelectedClassAbilityGrants(InAbilityGrants);
	GM->RefreshReadyStatus();
}

void ANexusPlayerController::Server_SetReady_Implementation(bool bReady)
{
	ANexusPlayerState* PS = GetPlayerState<ANexusPlayerState>();
	ANexusGameMode* GM = GetWorld()->GetAuthGameMode<ANexusGameMode>();

	if (!PS || !GM || !GM->IsClassSelectionOpen())
	{
		return;
	}

	PS->SetSelectionLockedIn(bReady);
	GM->RefreshReadyStatus();
}

void ANexusPlayerController::HandleClassSelectStateChanged()
{
	ANexusGameState* GS = GetWorld()->GetGameState<ANexusGameState>();
	if (!GS)
	{
		return;
	}

	if (GS->bClassSelectOpen)
	{
		HidePauseMenu();
		ShowClassSelectUI();
		ShowWaitingCameraForTeam();
	}
	else
	{
		HideClassSelectUI();
		ReturnToPawnCamera();
	}
}

void ANexusPlayerController::ApplyMenuInputMode(UUserWidget* FocusWidget)
{
	bShowMouseCursor = true;

	FInputModeUIOnly InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);

	if (FocusWidget)
	{
		const TSharedPtr<SWidget> SlateWidget = FocusWidget->GetCachedWidget();
		if (SlateWidget.IsValid() && SlateWidget->SupportsKeyboardFocus())
		{
			InputMode.SetWidgetToFocus(SlateWidget);
		}
	}

	SetInputMode(InputMode);
}

void ANexusPlayerController::RestoreGameplayInputMode()
{
	bShowMouseCursor = false;

	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);
}

void ANexusPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindFromObservedGameState();

	if (PauseMenuWidget)
	{
		PauseMenuWidget->RemoveFromParent();
		PauseMenuWidget = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}
