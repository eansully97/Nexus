#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NexusLocalTestQuickJoinWidget.generated.h"

class UButton;
class UTextBlock;
class UNexusGameInstance;

UCLASS(Blueprintable)
class NEXUS_API UNexusLocalTestQuickJoinWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintCallable, Category = "Local Test")
	void RefreshFromGameInstance();

private:
	UPROPERTY(meta = (BindWidgetOptional, AllowPrivateAccess = "true"), BlueprintReadOnly, Category = "Local Test")
	TObjectPtr<UTextBlock> TitleText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional, AllowPrivateAccess = "true"), BlueprintReadOnly, Category = "Local Test")
	TObjectPtr<UTextBlock> StatusText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional, AllowPrivateAccess = "true"), BlueprintReadOnly, Category = "Local Test")
	TObjectPtr<UButton> JoinButton = nullptr;

	UFUNCTION()
	void HandleJoinClicked();

	void BuildFallbackWidgetTree();
	void BindButtonHandlers();
	void UnbindButtonHandlers();
	UNexusGameInstance* GetNexusGameInstance() const;
};
