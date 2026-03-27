// Fill out your copyright notice in the Description page of Project Settings.


#include "NexusPlayerController.h"

#include "EnhancedInputSubsystems.h"
#include "Nexus/HUD/NexusHUD.h"
#include "Nexus/HUD/Widgets/NexusMainHUDWidget.h"

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