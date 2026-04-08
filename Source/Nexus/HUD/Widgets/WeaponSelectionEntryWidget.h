#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WeaponSelectionEntryWidget.generated.h"

class ANexusWeaponBase;
class UButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnWeaponSelectionEntryChosen,
	TSubclassOf<ANexusWeaponBase>,
	WeaponClass);

UCLASS()
class NEXUS_API UWeaponSelectionEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintCallable, Category="Weapon Entry")
	void InitializeWeaponEntry(TSubclassOf<ANexusWeaponBase> InWeaponClass);

	UFUNCTION(BlueprintCallable, Category="Weapon Entry")
	void SetEntrySelected(bool bInSelected);

	UFUNCTION(BlueprintCallable, Category="Weapon Entry")
	void SetEntryInteractionEnabled(bool bInEnabled);

	UFUNCTION(BlueprintPure, Category="Weapon Entry")
	TSubclassOf<ANexusWeaponBase> GetWeaponClass() const { return WeaponClass; }

	UFUNCTION(BlueprintPure, Category="Weapon Entry")
	bool IsSelected() const { return bSelected; }

	UFUNCTION(BlueprintPure, Category="Weapon Entry")
	bool IsInteractionEnabled() const { return bInteractionEnabled; }

	UPROPERTY(BlueprintAssignable, Category="Weapon Entry")
	FOnWeaponSelectionEntryChosen OnWeaponChosen;

protected:
	UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly)
	TObjectPtr<UButton> EntryButton = nullptr;

	UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly)
	TObjectPtr<UTextBlock> WeaponNameText = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon Entry")
	TSubclassOf<ANexusWeaponBase> WeaponClass = nullptr;

	UPROPERTY(BlueprintReadOnly, Category="Weapon Entry")
	bool bSelected = false;

	UPROPERTY(BlueprintReadOnly, Category="Weapon Entry")
	bool bInteractionEnabled = true;

	UFUNCTION(BlueprintImplementableEvent, Category="Weapon Entry")
	void BP_OnEntryVisualStateChanged(bool bInSelected, bool bInEnabled);

private:
	UFUNCTION()
	void HandleEntryButtonClicked();

	void RefreshEntryText();
	void RefreshEntryState();
	FText GetWeaponDisplayText() const;
};