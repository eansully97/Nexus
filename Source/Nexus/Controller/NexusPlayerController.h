// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Nexus/NexusEnumTypes.h"
#include "Nexus/DataAssets/CharacterClassInfo.h"
#include "NexusPlayerController.generated.h"


class UCameraComponent;
class UInputMappingContext;

/**
 * 
 */
UCLASS()
class NEXUS_API ANexusPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	
	void ShowWaitingCameraForTeam();
	void ReturnToPawnCamera();
	
	UFUNCTION(Client, Reliable)
	void ClientShowWaitingCamera();

	UFUNCTION(Client, Reliable)
	void ClientReturnToPawnCamera();

	ACameraActor* GetWaitingCameraActor(ENexusTeamID InTeamID);

	ENexusTeamID GetTeamID() const;

protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnRep_Pawn() override;
	virtual void Tick(float DeltaTime) override;
	virtual void OnRep_PlayerState() override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IMC")
	TArray<UInputMappingContext*> DefaultMappingContexts;
	
public:
	void RefreshHUDBindings();

	UFUNCTION(BlueprintCallable)
	void ShowClassSelectUI();

	UFUNCTION(BlueprintCallable)
	void HideClassSelectUI();
	
	void HandleClassSelectStateChanged();

	UFUNCTION(Server, Reliable)
	void Server_SelectClass(UCharacterClassInfo* InClassInfo);

	UFUNCTION(Server, Reliable)
	void Server_SetReady(bool bInReady);
	
private:
	UPROPERTY(BlueprintReadOnly, Replicated, Category="Crosshair", meta = (AllowPrivateAccess = "true"))
	FHitResult CurrentCrosshairHit;

	UPROPERTY(EditDefaultsOnly, Category="UI")
	TSubclassOf<UUserWidget> ClassSelectWidgetClass;

	UPROPERTY()
	TObjectPtr<UUserWidget> ClassSelectWidget;
	
public:
	UFUNCTION(BlueprintCallable, Category="Crosshair")
	bool GetCrosshairHitResult(FHitResult& OutHit, float TraceDistance) const;

	UFUNCTION(BlueprintPure)
	FHitResult GetCurrentCrosshairHit();

};
