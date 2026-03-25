// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Nexus/GameplayAbilitySystem/AttributeSet/BasicAttributeSet.h"
#include "NexusVitalsWidget.generated.h"

class ANexusCharacterBase;
class UAbilitySystemComponent;
class UProgressBar;
/**
 * 
 */
UCLASS()
class NEXUS_API UNexusVitalsWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintCallable, Category="Vitals")
	void SetObservedPawn(APawn* NewPawn);

protected:
	UPROPERTY(BlueprintReadOnly, Category="Vitals")
	TObjectPtr<ANexusCharacterBase> ObservedCharacter;

	UPROPERTY(BlueprintReadOnly, Category="Vitals")
	TObjectPtr<UAbilitySystemComponent> ObservedASC;

	UPROPERTY(BlueprintReadOnly, Category="Vitals")
	float Health = 0.f;

	UPROPERTY(BlueprintReadOnly, Category="Vitals")
	float MaxHealth = 0.f;

	UPROPERTY(BlueprintReadOnly, Category="Vitals")
	float Stamina = 0.f;

	UPROPERTY(BlueprintReadOnly, Category="Vitals")
	float MaxStamina = 0.f;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> HealthBar;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> StaminaBar;

	FDelegateHandle HealthChangedHandle;
	FDelegateHandle MaxHealthChangedHandle;
	FDelegateHandle StaminaChangedHandle;
	FDelegateHandle MaxStaminaChangedHandle;

	void BindToASC();
	void UnbindFromASC();
	void RefreshFromAttributes();
	void UpdateVitalsPercent();

	void HandleHealthChanged(const FOnAttributeChangeData& ChangeData);
	void HandleMaxHealthChanged(const FOnAttributeChangeData& ChangeData);
	void HandleStaminaChanged(const FOnAttributeChangeData& ChangeData);
	void HandleMaxStaminaChanged(const FOnAttributeChangeData& ChangeData);
};
