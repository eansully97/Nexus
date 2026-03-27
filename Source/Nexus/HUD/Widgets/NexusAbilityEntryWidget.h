#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayAbilitySpecHandle.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "NexusAbilityEntryWidget.generated.h"


class UNexusGameplayAbility;
class UAbilitySystemComponent;
class UTextBlock;
class UImage;

UCLASS()
class NEXUS_API UNexusAbilityEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintCallable, Category="Abilities")
	void InitializeAbilityEntry(AActor* InObservedActor, UAbilitySystemComponent* InASC, FGameplayAbilitySpecHandle InSpecHandle);

protected:
	UPROPERTY(BlueprintReadOnly, Category="Abilities")
	TObjectPtr<UAbilitySystemComponent> ObservedASC = nullptr;

	UPROPERTY(BlueprintReadOnly, Category="Abilities")
	TObjectPtr<AActor> ObservedActor = nullptr;

	FGameplayTagContainer ObservedCooldownTags;

	TArray<FDelegateHandle> CooldownTagEventHandles;
	
	UFUNCTION()
	void HandleCooldownGameplayTagChanged(const FGameplayTag GameplayTag, int32 NewCount);

	void BindCooldownListeners();
	void UnbindCooldownListeners();

	void UpdateAbilityBGColor(bool bIsActive) const;

	void BindListeners();
	void UnbindListeners();

	UPROPERTY(BlueprintReadOnly, Category="Abilities")
	FGameplayAbilitySpecHandle AbilitySpecHandle;

	UPROPERTY(BlueprintReadOnly, Category="Abilities")
	TObjectPtr<const UNexusGameplayAbility> ObservedAbility = nullptr;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly)
	TObjectPtr<UTextBlock> CooldownText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly)
	TObjectPtr<UImage> AbilityIcon = nullptr;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly)
	TObjectPtr<UImage> CooldownOverlay = nullptr;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly)
	TObjectPtr<UImage> CooldownBG = nullptr;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly)
	TObjectPtr<UImage> AbilityBG = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Abilities")
	float CooldownRefreshRate = 0.1f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Abilities")
	float ActiveStateRefreshRate = 0.05f;

	FTimerHandle CooldownTimerHandle;
	FTimerHandle ActiveStateTimerHandle;

	void RefreshFromAbility();
	void UpdateCooldownProgress();
	void ResetCooldownVisuals();

	void UpdateAbilityActiveState();
	bool IsObservedAbilityActive() const;

	UFUNCTION(BlueprintImplementableEvent, Category="Abilities")
	void BP_OnAbilityInitialized();

	UFUNCTION(BlueprintImplementableEvent, Category="Abilities")
	void BP_UpdateCooldownVisuals(float TimeRemaining);

	UFUNCTION(BlueprintImplementableEvent, Category="Abilities")
	void BP_OnCooldownFinished();
};