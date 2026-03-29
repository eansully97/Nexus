// Fill out your copyright notice in the Description page of Project Settings.


#include "NexusPlayerController.h"

#include "EnhancedInputSubsystems.h"
#include "Camera/CameraActor.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Nexus/GameMode/NexusGameMode.h"
#include "Nexus/GameState/NexusGameState.h"
#include "Nexus/HUD/NexusHUD.h"
#include "Nexus/HUD/Widgets/NexusMainHUDWidget.h"
#include "Nexus/PlayerState/NexusPlayerState.h"

ACameraActor* ANexusPlayerController::GetWaitingCameraActor(ENexusTeamID InTeamID)
{
	FName DesiredTag = NAME_None;

	switch (InTeamID)
	{
	case ENexusTeamID::TeamA:
		DesiredTag = FName("TeamA");
		break;

	case ENexusTeamID::TeamB:
		DesiredTag = FName("TeamB");
		break;

	default:
		return nullptr;
	}

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), DesiredTag, FoundActors);

	if (FoundActors.Num() == 0)
	{
		return nullptr;
	}

	return Cast<ACameraActor>(FoundActors[0]);
}

ENexusTeamID ANexusPlayerController::GetTeamID() const
{
	return GetPlayerState<ANexusPlayerState>()->GetTeamID();
}

void ANexusPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// Add Input Mapping Contexts
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
		{
			Subsystem->AddMappingContext(CurrentContext, 0);
		}
	}
}

void ANexusPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	RefreshHUDBindings();
}

void ANexusPlayerController::OnRep_Pawn()
{
	Super::OnRep_Pawn();
	RefreshHUDBindings();
}

void ANexusPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	GetCrosshairHitResult(CurrentCrosshairHit, 10000.f);
}

void ANexusPlayerController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	
	ShowWaitingCameraForTeam();
}

void ANexusPlayerController::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ThisClass, CurrentCrosshairHit)
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

	FVector WorldLocation;
	FVector WorldDirection;

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

FHitResult ANexusPlayerController::GetCurrentCrosshairHit()
{
	return CurrentCrosshairHit;
}

void ANexusPlayerController::ShowWaitingCameraForTeam()
{
	ACameraActor* WaitingCamera = GetWaitingCameraActor(GetTeamID());

	if (!WaitingCamera)
	{
		UE_LOG(LogTemp, Warning, TEXT("ShowWaitingCameraForTeam: No waiting camera found for TeamId %d on %s"), GetTeamID(), *GetName());
		return;
	}

	SetControlRotation(WaitingCamera->GetActorRotation());
	SetViewTargetWithBlend(WaitingCamera, 0.f);

	UE_LOG(LogTemp, Warning, TEXT("ShowWaitingCameraForTeam: Using camera %s for TeamId %d on %s"),
		*GetNameSafe(WaitingCamera),
		GetTeamID(),
		*GetName());
}

void ANexusPlayerController::ReturnToPawnCamera()
{
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		UE_LOG(LogTemp, Warning, TEXT("ReturnToPawnCamera: No pawn on %s"), *GetName());
		return;
	}
	HideClassSelectUI();
	SetControlRotation(ControlledPawn->GetActorRotation());
	SetViewTargetWithBlend(ControlledPawn, 0.f);
}

void ANexusPlayerController::ClientShowWaitingCamera_Implementation()
{
	ShowWaitingCameraForTeam();
}

void ANexusPlayerController::ClientReturnToPawnCamera_Implementation()
{
	ReturnToPawnCamera();
}

void ANexusPlayerController::ShowClassSelectUI()
{
	if (!IsLocalController() || !ClassSelectWidgetClass || ClassSelectWidget)
	{
		return;
	}

	ClassSelectWidget = CreateWidget<UUserWidget>(this, ClassSelectWidgetClass);
	if (ClassSelectWidget)
	{
		ClassSelectWidget->AddToViewport();
		bShowMouseCursor = true;

		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(ClassSelectWidget->TakeWidget());
		SetInputMode(InputMode);
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

	bShowMouseCursor = false;

	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);
	
}

void ANexusPlayerController::Server_SelectClass_Implementation(UCharacterClassInfo* ClassInfo)
{
	ANexusPlayerState* PS = GetPlayerState<ANexusPlayerState>();
	ANexusGameMode* GM = GetWorld()->GetAuthGameMode<ANexusGameMode>();

	if (!PS || !GM || !GM->IsClassSelectionOpen())
	{
		return;
	}

	if (!GM->IsValidClassChoice(ClassInfo))
	{
		return;
	}

	PS->SetCharacterClassInfo(ClassInfo);
	PS->SetSelectionLockedIn(false);

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

	if (!PS->GetCharacterClassInfo())
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
		ShowClassSelectUI();
	}
	else
	{
		HideClassSelectUI();
	}
}