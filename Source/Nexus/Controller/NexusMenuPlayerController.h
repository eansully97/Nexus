#pragma once


#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "NexusMenuPlayerController.generated.h"


class UUserWidget;
class UNexusGameInstance;
class UNexusLocalTestQuickJoinWidget;
class UNexusReconnectPromptWidget;

UCLASS()
class NEXUS_API ANexusMenuPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ANexusMenuPlayerController();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UUserWidget> MainMenuWidgetClass;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	TObjectPtr<UUserWidget> MainMenuWidget;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UNexusReconnectPromptWidget> ReconnectPromptWidgetClass;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	TObjectPtr<UNexusReconnectPromptWidget> ReconnectPromptWidget;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UNexusLocalTestQuickJoinWidget> LocalTestQuickJoinWidgetClass;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	TObjectPtr<UNexusLocalTestQuickJoinWidget> LocalTestQuickJoinWidget;

	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowMainMenu();

	UFUNCTION(BlueprintCallable, Category = "UI")
	void HideMainMenu();

	UFUNCTION(BlueprintCallable, Category = "UI")
	void RefreshReconnectPrompt();

	UFUNCTION(BlueprintCallable, Category = "UI")
	void RefreshLocalTestQuickJoinWidget();

	UFUNCTION()
	void HandleReconnectStateChanged();

	UNexusGameInstance* GetNexusGameInstance() const;
	void ApplyMenuInputMode(UUserWidget* FocusWidget = nullptr);
};
