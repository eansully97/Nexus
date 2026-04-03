#pragma once


#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ANexusMenuPlayerController.generated.h"


class UUserWidget;

UCLASS()
class NEXUS_API ANexusMenuPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ANexusMenuPlayerController();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UUserWidget> MainMenuWidgetClass;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	TObjectPtr<UUserWidget> MainMenuWidget;

	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowMainMenu();

	UFUNCTION(BlueprintCallable, Category = "UI")
	void HideMainMenu();

	void ApplyMenuInputMode();
};