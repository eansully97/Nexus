#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ClassSelectionEntryWidget.generated.h"

class UButton;
class UTextBlock;
class UCharacterClassInfo;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnClassSelectionEntryChosen, UCharacterClassInfo*, ClassInfo);

UCLASS()
class NEXUS_API UClassSelectionEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintCallable, Category="Class Entry")
	void InitializeClassEntry(UCharacterClassInfo* InClassInfo);

	UFUNCTION(BlueprintCallable, Category="Class Entry")
	void SetEntrySelected(bool bInSelected);

	UFUNCTION(BlueprintCallable, Category="Class Entry")
	void SetEntryInteractionEnabled(bool bInEnabled);

	UFUNCTION(BlueprintPure, Category="Class Entry")
	UCharacterClassInfo* GetClassInfo() const { return ClassInfo; }

	UPROPERTY(BlueprintAssignable, Category="Class Entry")
	FOnClassSelectionEntryChosen OnClassChosen;

protected:
	UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly)
	TObjectPtr<UButton> EntryButton = nullptr;

	UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly)
	TObjectPtr<UTextBlock> ClassNameText = nullptr;

	UPROPERTY(BlueprintReadOnly, Category="Class Entry")
	TObjectPtr<UCharacterClassInfo> ClassInfo = nullptr;

	UPROPERTY(BlueprintReadOnly, Category="Class Entry")
	bool bSelected = false;

	UPROPERTY(BlueprintReadOnly, Category="Class Entry")
	bool bInteractionEnabled = true;

	UFUNCTION(BlueprintImplementableEvent, Category="Class Entry")
	void BP_OnEntryVisualStateChanged(bool bInSelected, bool bInEnabled);

private:
	UFUNCTION()
	void HandleEntryButtonClicked();

	void RefreshEntryText();
	void RefreshEntryState();
	FText GetClassDisplayText() const;
};