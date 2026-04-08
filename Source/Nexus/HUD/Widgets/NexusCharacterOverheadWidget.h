#pragma once

#include "CoreMinimal.h"
#include "Nexus/HUD/Widgets/NexusVitalsWidget.h"
#include "NexusCharacterOverheadWidget.generated.h"

class ANexusCharacterBase;
class UBorder;
class UTextBlock;

UCLASS(Blueprintable)
class NEXUS_API UNexusCharacterOverheadWidget : public UNexusVitalsWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintCallable, Category="Overhead")
	void SetObservedCharacter(ANexusCharacterBase* NewCharacter);

protected:
	virtual bool ShouldAutoObserveOwningPawn() const override { return false; }

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Overhead|Style")
	FLinearColor TeamAHealthColor = FLinearColor(0.94f, 0.29f, 0.29f, 1.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Overhead|Style")
	FLinearColor TeamBHealthColor = FLinearColor(0.29f, 0.56f, 0.94f, 1.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Overhead|Style")
	FLinearColor NeutralHealthColor = FLinearColor(0.75f, 0.75f, 0.75f, 1.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Overhead|Style")
	FLinearColor AllyHealthColor = FLinearColor(0.18f, 0.82f, 0.33f, 1.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Overhead|Style")
	FLinearColor EnemyHealthColor = FLinearColor(0.90f, 0.18f, 0.18f, 1.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Overhead|Style")
	float PlayerHealthBarWidthScale = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Overhead|Style")
	float MinionHealthBarWidthScale = 1.65f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Overhead|Style")
	float PlayerHealthBarHeightScale = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Overhead|Style")
	float MinionHealthBarHeightScale = 0.72f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Overhead|Style")
	float PlayerHorizontalPadding = 8.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Overhead|Style")
	float PlayerVerticalPadding = 4.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Overhead|Style")
	float MinionHorizontalPadding = 4.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Overhead|Style")
	float MinionVerticalPadding = 2.0f;

	UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly, Category="Overhead")
	TObjectPtr<UBorder> RootBorder = nullptr;

	UPROPERTY(meta=(BindWidgetOptional), BlueprintReadOnly, Category="Overhead")
	TObjectPtr<UTextBlock> NameText = nullptr;

private:
	UFUNCTION()
	void HandleObservedTeamChanged();

	void BindToObservedCharacter();
	void UnbindFromObservedCharacter();
	void RefreshFromCharacter();
	void RefreshText();
	void RefreshHealthBar();
	void RefreshLayout();
	void RefreshTeamPresentation();
	void BuildFallbackWidgetTree();
};
