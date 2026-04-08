#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NexusMainHUDWidget.generated.h"

class ANexusCharacterBase;
class UNexusVitalsWidget;
class UNexusAbilityContainer;

UCLASS()
class NEXUS_API UNexusMainHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="HUD")
	virtual void SetObservedPawn(APawn* NewPawn);

protected:
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly)
	TObjectPtr<UNexusVitalsWidget> VitalsWidget = nullptr;

	UPROPERTY(BlueprintReadOnly, Category="HUD")
	TObjectPtr<ANexusCharacterBase> ObservedCharacter = nullptr;

	void PropagateObservedPawnToChildWidgets(APawn* NewPawn);
};