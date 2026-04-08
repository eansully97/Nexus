#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Nexus/NexusAbilityGrant.h"
#include "ClassSelectionWidget.generated.h"

class ANexusPlayerController;
class ANexusGameState;
class ANexusPlayerState;
class ANexusWeaponBase;
class UButton;
class UCharacterClassInfo;
class UClassSelectionEntryWidget;
class UScrollBox;
class UTextBlock;
class UWeaponSelectionEntryWidget;
class UWidgetSwitcher;

UCLASS()
class NEXUS_API UClassSelectionWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintCallable, Category="Selection")
	void SelectClass(UCharacterClassInfo* InClassInfo);

	UFUNCTION(BlueprintCallable, Category="Selection")
	void SelectWeapon(TSubclassOf<ANexusWeaponBase> InWeaponClass);

	UFUNCTION(BlueprintCallable, Category="Selection")
	void SetSelectedClassAbilities(const TArray<FNexusAbilityGrant>& InAbilityGrants);

	UFUNCTION(BlueprintCallable, Category="Selection")
	void ClickReady();

	UFUNCTION(BlueprintCallable, Category="Selection")
	void ClickUnready();

	UFUNCTION(BlueprintCallable, Category="Selection")
	void BindToObservedPlayerState();

	UFUNCTION(BlueprintCallable, Category="Selection")
	void BindToObservedGameState();

	UFUNCTION(BlueprintCallable, Category="Selection")
	void RefreshFromObservedState();

	UFUNCTION(BlueprintCallable, Category="Selection")
	void BuildClassPage();

	UFUNCTION(BlueprintCallable, Category="Selection")
	void BuildWeaponPageFromClass(UCharacterClassInfo* InClassInfo);

	UFUNCTION(BlueprintCallable, Category="Selection")
	void ShowClassPage();

	UFUNCTION(BlueprintCallable, Category="Selection")
	void ShowWeaponPage();

	UFUNCTION(BlueprintPure, Category="Selection")
	UCharacterClassInfo* GetSelectedClass() const;

	UFUNCTION(BlueprintPure, Category="Selection")
	TSubclassOf<ANexusWeaponBase> GetSelectedWeapon() const;

	UFUNCTION(BlueprintPure, Category="Selection")
	TArray<FNexusAbilityGrant> GetSelectedClassAbilities() const;

	UFUNCTION(BlueprintPure, Category="Selection")
	bool GetIsReadyLockedIn() const;

	UFUNCTION(BlueprintPure, Category="Selection")
	bool CanClickReady() const;

	UFUNCTION(BlueprintPure, Category="Selection")
	bool IsClassSelected(UCharacterClassInfo* InClassInfo) const;

	UFUNCTION(BlueprintPure, Category="Selection")
	bool IsWeaponSelected(TSubclassOf<ANexusWeaponBase> InWeaponClass) const;

	UFUNCTION(BlueprintImplementableEvent, Category="Selection")
	void BP_OnSelectionStateRefreshed(
		UCharacterClassInfo* SelectedClass,
		TSubclassOf<ANexusWeaponBase> SelectedWeapon,
		const TArray<FNexusAbilityGrant>& SelectedAbilities,
		bool bLockedIn);

protected:
	UPROPERTY(BlueprintReadOnly, Category="Selection")
	TObjectPtr<ANexusPlayerState> ObservedPlayerState = nullptr;

	UPROPERTY(BlueprintReadOnly, Category="Selection")
	TObjectPtr<ANexusGameState> ObservedGameState = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Selection|Classes")
	TSubclassOf<UClassSelectionEntryWidget> ClassEntryWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Selection|Weapons")
	TSubclassOf<UWeaponSelectionEntryWidget> WeaponEntryWidgetClass;

	UPROPERTY(BlueprintReadOnly, Category="Selection")
	UCharacterClassInfo* DisplayedClassInfo = nullptr;

	UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly)
	TObjectPtr<UWidgetSwitcher> WidgetSwitcher_1 = nullptr;

	UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly)
	TObjectPtr<UButton> ReadyButton = nullptr;

	UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly)
	TObjectPtr<UButton> BackButton = nullptr;

	UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly)
	TObjectPtr<UScrollBox> ClassListBox = nullptr;

	UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly)
	TObjectPtr<UScrollBox> WeaponListBox = nullptr;

	UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly)
	TObjectPtr<UTextBlock> SelectedClassText = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Selection|Pages")
	int32 ClassPageIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Selection|Pages")
	int32 WeaponPageIndex = 1;

private:
	UPROPERTY()
	TArray<TObjectPtr<UClassSelectionEntryWidget>> CurrentClassEntries;

	UPROPERTY()
	TArray<TObjectPtr<UWeaponSelectionEntryWidget>> CurrentWeaponEntries;

	UFUNCTION()
	void HandleObservedPlayerProfileChanged();

	UFUNCTION()
	void HandleObservedAvailableClassesChanged();

	UFUNCTION()
	void HandleReadyButtonClicked();

	UFUNCTION()
	void HandleBackButtonClicked();

	UFUNCTION()
	void HandleClassEntryChosen(UCharacterClassInfo* InClassInfo);

	UFUNCTION()
	void HandleWeaponEntryChosen(TSubclassOf<ANexusWeaponBase> InWeaponClass);

	ANexusPlayerController* GetNexusPC() const;
	ANexusPlayerState* ResolvePlayerState() const;
	ANexusGameState* ResolveGameState() const;
	void UnbindFromObservedPlayerState();
	void UnbindFromObservedGameState();

	void BindNativeButtonHandlers();
	void UnbindNativeButtonHandlers();

	void ClearClassEntryWidgets();
	void ClearWeaponEntryWidgets();

	void RebuildClassEntryWidgets();
	void RebuildWeaponEntryWidgets(UCharacterClassInfo* InClassInfo);

	void RefreshClassEntryStates(
		UCharacterClassInfo* SelectedClass,
		bool bLockedIn);

	void ResolveWeaponOptionsForClass(
		UCharacterClassInfo* InClassInfo,
		TArray<TSubclassOf<ANexusWeaponBase>>& OutWeaponClasses) const;

	void RefreshWeaponEntryStates(
		TSubclassOf<ANexusWeaponBase> SelectedWeaponClass,
		bool bLockedIn);

	void RefreshNativeControlStates(
		UCharacterClassInfo* SelectedClass,
		TSubclassOf<ANexusWeaponBase> SelectedWeaponClass,
		bool bLockedIn);

	FText GetClassDisplayText(UCharacterClassInfo* InClassInfo) const;
};
