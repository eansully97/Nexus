// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "NexusPlayerController.generated.h"

class UInputMappingContext;

/**
 * 
 */
UCLASS()
class NEXUS_API ANexusPlayerController : public APlayerController
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IMC")
	TArray<UInputMappingContext*> DefaultMappingContexts;

protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnRep_Pawn() override;

	void RefreshHUDBindings();
};
