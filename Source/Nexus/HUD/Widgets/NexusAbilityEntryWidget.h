#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayAbilitySpecHandle.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "NexusAbilityEntryWidget.generated.h"

class ANexusCharacterBase;
class ANexusPlayerController;
class UNexusGameplayAbility;
class UAbilitySystemComponent;
class UImage;
class UTextBlock;

UCLASS()
class NEXUS_API UNexusAbilityEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintCallable, Category="Abilities")
	void InitializeAbilityEntry(AActor* InObservedActor, UAbilitySystemComponent* InASC, FGameplayAbilitySpecHandle InSpecHandle);

	UFUNCTION(BlueprintPure, Category="Abilities")
	FGameplayAbilitySpecHandle GetAbilitySpecHandle() const { return AbilitySpecHandle; }

protected:
	UPROPERTY(BlueprintReadOnly, Category="Abilities")
	TObjectPtr<UAbilitySystemComponent> ObservedASC = nullptr;

	UPROPERTY(BlueprintReadOnly, Category="Abilities")
	TObjectPtr<AActor> ObservedActor = nullptr;

	UPROPERTY(BlueprintReadOnly, Category="Abilities")
	TObjectPtr<ANexusPlayerController> ObservedPlayerController = nullptr;

	FGameplayTagContainer ObservedCooldownTags;
	TArray<FDelegateHandle> CooldownTagEventHandles;

	UFUNCTION()
	void HandleCooldownGameplayTagChanged(const FGameplayTag GameplayTag, int32 NewCount);

	UFUNCTION()
	void HandleControllerTargetChanged(ANexusCharacterBase* NewTarget, bool bHasValidTarget);

	void BindCooldownListeners();
	void UnbindCooldownListeners();

	void BindTargetingListener();
	void UnbindTargetingListener();

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
	TObjectPtr<UImage> TargetRequiredOverlay = nullptr;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly)
	TObjectPtr<UImage> CooldownBG = nullptr;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly)
	TObjectPtr<UImage> AbilityBG = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Abilities")
	float CooldownRefreshRate = 0.1f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Abilities")
	float ActiveStateRefreshRate = 0.05f;

	UPROPERTY(BlueprintReadOnly, Category="Abilities")
	bool bHasValidTargetForObservedAbility = true;

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

	void UpdateTargetRequirementState();
	bool DoesAbilityNeedValidTarget() const;
	bool IsLocalTargetUsableForObservedAbility() const;

	UFUNCTION(BlueprintImplementableEvent, Category="Abilities")
	void BP_OnTargetRequirementChanged(bool bHasValidTarget);
};