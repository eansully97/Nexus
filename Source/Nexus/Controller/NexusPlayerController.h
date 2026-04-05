#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Nexus/NexusEnumTypes.h"
#include "Nexus/DataAssets/CharacterClassInfo.h"
#include "NexusPlayerController.generated.h"

class ACameraActor;
class UInputMappingContext;
class UUserWidget;
class ANexusCharacterBase;
class UNexusGameplayAbility;
class ANexusWeaponBase;

UCLASS()
class NEXUS_API ANexusPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sensitivity")
	float LookSensitivity{1.f};

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IMC")
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
	void Server_SelectWeapon(TSubclassOf<ANexusWeaponBase> InWeaponClass);

	UFUNCTION(Server, Reliable)
	void Server_SetSelectedClassAbilities(const TArray<FNexusAbilityGrant>& InAbilityGrants);

	UFUNCTION(Server, Reliable)
	void Server_SetReady(bool bInReady);

private:
	UPROPERTY(BlueprintReadOnly, Replicated, Category="Crosshair", meta=(AllowPrivateAccess="true"))
	FHitResult CurrentCrosshairHit;

	UPROPERTY(BlueprintReadOnly, Category="Targeting", meta=(AllowPrivateAccess="true"))
	TObjectPtr<ANexusCharacterBase> CurrentTargetedCharacter = nullptr;

	UPROPERTY(BlueprintReadOnly, Category="Targeting", meta=(AllowPrivateAccess="true"))
	bool bHasValidTarget = false;

	UPROPERTY(EditDefaultsOnly, Category="UI")
	TSubclassOf<UUserWidget> ClassSelectWidgetClass;

	UPROPERTY()
	TObjectPtr<UUserWidget> ClassSelectWidget;

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