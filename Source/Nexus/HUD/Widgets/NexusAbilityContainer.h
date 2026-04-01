#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayAbilitySpecHandle.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Nexus/NexusEnumTypes.h"
#include "NexusAbilityContainer.generated.h"


class ANexusCharacterBase;
class UAbilitySystemComponent;
class UHorizontalBox;
class UNexusAbilityEntryWidget;

UCLASS()
class NEXUS_API UNexusAbilityContainer : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintCallable, Category="Abilities")
	void SetObservedPawn(APawn* NewPawn);
	void BindAbilitiesChangedEvent();
	void UnbindAbilitiesChangedEvent();
	UFUNCTION()
	void HandleAbilitiesChanged();

	UFUNCTION(BlueprintCallable, Category="Abilities")
	void RefreshContainer();

protected:
	UPROPERTY(BlueprintReadOnly, Category="Abilities")
	TObjectPtr<ANexusCharacterBase> ObservedCharacter = nullptr;

	UPROPERTY(BlueprintReadOnly, Category="Abilities")
	TObjectPtr<UAbilitySystemComponent> ObservedASC = nullptr;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly)
	TObjectPtr<UHorizontalBox> AbilityContainer = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Abilities")
	TSubclassOf<UNexusAbilityEntryWidget> AbilityEntryWidgetClass;

	UPROPERTY(BlueprintReadOnly, Category="Abilities")
	TArray<FGameplayAbilitySpecHandle> AbilitiesForContainer;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Abilities")
	EAbilityContainerInfo ContainerType = EAbilityContainerInfo::None;

	void GetAbilitiesForContainer();
	void RebuildContainer();
};