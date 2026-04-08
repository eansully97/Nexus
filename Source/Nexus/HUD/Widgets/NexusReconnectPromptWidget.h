#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NexusReconnectPromptWidget.generated.h"

class UButton;
class UTextBlock;
class UNexusGameInstance;

UCLASS(Blueprintable)
class NEXUS_API UNexusReconnectPromptWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintCallable, Category = "Reconnect")
	void RefreshFromGameInstance();

protected:
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Reconnect")
	TObjectPtr<UTextBlock> TitleText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Reconnect")
	TObjectPtr<UTextBlock> StatusText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Reconnect")
	TObjectPtr<UButton> ReconnectButton = nullptr;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Reconnect")
	TObjectPtr<UButton> DismissButton = nullptr;

private:
	UFUNCTION()
	void HandleReconnectClicked();

	UFUNCTION()
	void HandleDismissClicked();

	UFUNCTION()
	void HandleReconnectStateChanged();

	void BuildFallbackWidgetTree();
	void BindButtonHandlers();
	void UnbindButtonHandlers();
	UNexusGameInstance* GetNexusGameInstance() const;
};
