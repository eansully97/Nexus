#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Nexus/NexusEnumTypes.h"
#include "Nexus/DataAssets/CharacterClassInfo.h"
#include "NexusPlayerController.generated.h"

class ACameraActor;
class UInputMappingContext;
class UClassSelectionWidget;
class UUserWidget;
class ANexusCharacterBase;
class UNexusGameplayAbility;
class ANexusWeaponBase;
class ANexusGameState;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnControllerTargetChanged,
	ANexusCharacterBase*,
	NewTarget,
	bool,
	bHasValidTarget);

UCLASS()
class NEXUS_API ANexusPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ANexusPlayerController();
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sensitivity")
	float LookSensitivity{1.f};

	void ShowWaitingCameraForTeam();
	void ReturnToPawnCamera();

	UFUNCTION(Client, Reliable)
	void ClientRefreshClassSelectState();

	UFUNCTION(Client, Reliable)
	void ClientCacheReconnectToken(const FString& ReconnectToken);

	ACameraActor* GetWaitingCameraActor(ENexusTeamID InTeamID) const;
	
	ENexusTeamID GetTeamID() const;

	UPROPERTY(BlueprintAssignable, Category="Targeting")
	FOnControllerTargetChanged OnControllerTargetChanged;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnRep_Pawn() override;
	virtual void PawnLeavingGame() override;
	virtual void Tick(float DeltaTime) override;
	virtual void OnRep_PlayerState() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IMC")
	TArray<UInputMappingContext*> DefaultMappingContexts;

public:
	void RefreshHUDBindings();

	UFUNCTION(BlueprintCallable)
	void ShowClassSelectUI();

	UFUNCTION(BlueprintCallable)
	void HideClassSelectUI();

	UFUNCTION(BlueprintCallable, Category="UI")
	void TogglePauseMenu();

	UFUNCTION(BlueprintCallable, Category="UI")
	void ShowPauseMenu();

	UFUNCTION(BlueprintCallable, Category="UI")
	void HidePauseMenu();

	UFUNCTION(BlueprintPure, Category="UI")
	bool IsPauseMenuOpen() const { return PauseMenuWidget != nullptr; }

	void HandleClassSelectStateChanged();


	UFUNCTION(Server, Reliable)
	void Server_SelectClass(UCharacterClassInfo* InClassInfo);

	UFUNCTION(Server, Reliable)
	void Server_SelectWeapon(TSubclassOf<ANexusWeaponBase> InWeaponClass);

	UFUNCTION(Server, Reliable)
	void Server_SetSelectedClassAbilities(const TArray<FNexusAbilityGrant>& InAbilityGrants);

	UFUNCTION(Server, Reliable)
	void Server_SetReady(bool bInReady);

private:
	UPROPERTY(BlueprintReadOnly, Category="Crosshair", meta=(AllowPrivateAccess="true"))
	FHitResult CurrentCrosshairHit;

	UPROPERTY(BlueprintReadOnly, Category="Targeting", meta=(AllowPrivateAccess="true"))
	TObjectPtr<ANexusCharacterBase> CurrentTargetedCharacter = nullptr;

	UPROPERTY(BlueprintReadOnly, Category="Targeting", meta=(AllowPrivateAccess="true"))
	bool bHasValidTarget = false;

	UPROPERTY(EditDefaultsOnly, Category="UI")
	TSubclassOf<UClassSelectionWidget> ClassSelectWidgetClass;

	UPROPERTY()
	TObjectPtr<UClassSelectionWidget> ClassSelectWidget;

	UPROPERTY(EditDefaultsOnly, Category="UI")
	TSubclassOf<UUserWidget> PauseMenuWidgetClass;

	UPROPERTY()
	TObjectPtr<UUserWidget> PauseMenuWidget;

	UPROPERTY()
	TObjectPtr<ANexusGameState> ObservedGameState = nullptr;

	void BindClassSelectWidgetToPlayerState();
	void ApplyMenuInputMode(UUserWidget* FocusWidget);
	void RestoreGameplayInputMode();
	void BroadcastTargetChangedIfNeeded(
		ANexusCharacterBase* PreviousTarget,
		bool bPreviousHasValidTarget);
	
	void BindToObservedGameState();
	void UnbindFromObservedGameState();

	UFUNCTION()
	void HandleObservedClassSelectOpenChanged(bool bIsOpen);

	UFUNCTION()
	void HandleObservedMatchEndedChanged(bool bHasEnded, ENexusTeamID WinningTeam);

	void ShowMatchEndedMessage(ENexusTeamID WinningTeam);

public:
	UFUNCTION(BlueprintCallable, Category="Crosshair")
	bool GetCrosshairHitResult(FHitResult& OutHit, float TraceDistance) const;

	UFUNCTION(BlueprintPure, Category="Crosshair")
	FHitResult GetCurrentCrosshairHit() const;

	UFUNCTION(BlueprintPure, Category="Targeting")
	ANexusCharacterBase* GetCurrentTargetedCharacter() const { return CurrentTargetedCharacter; }

	UFUNCTION(BlueprintPure, Category="Targeting")
	bool HasValidTarget() const { return bHasValidTarget; }

	UFUNCTION(BlueprintPure, Category="Targeting")
	bool IsValidTargetCharacter(ANexusCharacterBase* SourceCharacter, ANexusCharacterBase* TargetCharacter) const;

	UFUNCTION(BlueprintPure, Category="Targeting")
	bool HasUsableTargetForAbility(const UNexusGameplayAbility* Ability) const;

	UFUNCTION(BlueprintPure, Category="Targeting")
	ANexusCharacterBase* GetUsableTargetForAbility(const UNexusGameplayAbility* Ability) const;

private:
	void UpdateTargetedCharacter();
};
