// Fill out your copyright notice in the Description page of Project Settings.


#include "NexusVitalsWidget.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Components/ProgressBar.h"
#include "Nexus/Character/NexusCharacterBase.h"

void UNexusVitalsWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Optional initial bind for first spawn.
	if (APawn* OwningPawn = GetOwningPlayerPawn())
	{
		SetObservedPawn(OwningPawn);
	}
}

void UNexusVitalsWidget::NativeDestruct()
{
	UnbindFromASC();
	Super::NativeDestruct();
}

void UNexusVitalsWidget::SetObservedPawn(APawn* NewPawn)
{
	ANexusCharacterBase* NewCharacter = Cast<ANexusCharacterBase>(NewPawn);

	if (ObservedCharacter == NewCharacter)
	{
		return;
	}

	UnbindFromASC();

	ObservedCharacter = NewCharacter;
	ObservedASC = nullptr;

	if (!ObservedCharacter)
	{
		Health = 0.f;
		MaxHealth = 0.f;
		Stamina = 0.f;
		MaxStamina = 0.f;
		UpdateVitalsPercent();
		return;
	}

	ObservedASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(ObservedCharacter);

	if (!ObservedASC)
	{
		Health = 0.f;
		MaxHealth = 0.f;
		Stamina = 0.f;
		MaxStamina = 0.f;
		UpdateVitalsPercent();
		return;
	}

	RefreshFromAttributes();
	BindToASC();
	UpdateVitalsPercent();
}

void UNexusVitalsWidget::BindToASC()
{
	if (!ObservedASC)
	{
		return;
	}

	HealthChangedHandle = ObservedASC
		->GetGameplayAttributeValueChangeDelegate(UBasicAttributeSet::GetHealthAttribute())
		.AddUObject(this, &UNexusVitalsWidget::HandleHealthChanged);

	MaxHealthChangedHandle = ObservedASC
		->GetGameplayAttributeValueChangeDelegate(UBasicAttributeSet::GetMaxHealthAttribute())
		.AddUObject(this, &UNexusVitalsWidget::HandleMaxHealthChanged);

	StaminaChangedHandle = ObservedASC
		->GetGameplayAttributeValueChangeDelegate(UBasicAttributeSet::GetStaminaAttribute())
		.AddUObject(this, &UNexusVitalsWidget::HandleStaminaChanged);

	MaxStaminaChangedHandle = ObservedASC
		->GetGameplayAttributeValueChangeDelegate(UBasicAttributeSet::GetMaxStaminaAttribute())
		.AddUObject(this, &UNexusVitalsWidget::HandleMaxStaminaChanged);
}

void UNexusVitalsWidget::UnbindFromASC()
{
	if (!ObservedASC)
	{
		return;
	}

	ObservedASC->GetGameplayAttributeValueChangeDelegate(UBasicAttributeSet::GetHealthAttribute())
		.Remove(HealthChangedHandle);

	ObservedASC->GetGameplayAttributeValueChangeDelegate(UBasicAttributeSet::GetMaxHealthAttribute())
		.Remove(MaxHealthChangedHandle);

	ObservedASC->GetGameplayAttributeValueChangeDelegate(UBasicAttributeSet::GetStaminaAttribute())
		.Remove(StaminaChangedHandle);

	ObservedASC->GetGameplayAttributeValueChangeDelegate(UBasicAttributeSet::GetMaxStaminaAttribute())
		.Remove(MaxStaminaChangedHandle);

	HealthChangedHandle.Reset();
	MaxHealthChangedHandle.Reset();
	StaminaChangedHandle.Reset();
	MaxStaminaChangedHandle.Reset();
}

void UNexusVitalsWidget::RefreshFromAttributes()
{
	if (!ObservedASC)
	{
		return;
	}

	Health = ObservedASC->GetNumericAttribute(UBasicAttributeSet::GetHealthAttribute());
	MaxHealth = ObservedASC->GetNumericAttribute(UBasicAttributeSet::GetMaxHealthAttribute());
	Stamina = ObservedASC->GetNumericAttribute(UBasicAttributeSet::GetStaminaAttribute());
	MaxStamina = ObservedASC->GetNumericAttribute(UBasicAttributeSet::GetMaxStaminaAttribute());
}

void UNexusVitalsWidget::UpdateVitalsPercent()
{
	const float HealthPercent = (MaxHealth > 0.f) ? (Health / MaxHealth) : 0.f;
	const float StaminaPercent = (MaxStamina > 0.f) ? (Stamina / MaxStamina) : 0.f;

	if (HealthBar)
	{
		HealthBar->SetPercent(HealthPercent);
	}

	if (StaminaBar)
	{
		StaminaBar->SetPercent(StaminaPercent);
	}
}

void UNexusVitalsWidget::HandleHealthChanged(const FOnAttributeChangeData& ChangeData)
{
	Health = ChangeData.NewValue;
	UpdateVitalsPercent();
}

void UNexusVitalsWidget::HandleMaxHealthChanged(const FOnAttributeChangeData& ChangeData)
{
	MaxHealth = ChangeData.NewValue;
	UpdateVitalsPercent();
}

void UNexusVitalsWidget::HandleStaminaChanged(const FOnAttributeChangeData& ChangeData)
{
	Stamina = ChangeData.NewValue;
	UpdateVitalsPercent();
}

void UNexusVitalsWidget::HandleMaxStaminaChanged(const FOnAttributeChangeData& ChangeData)
{
	MaxStamina = ChangeData.NewValue;
	UpdateVitalsPercent();
}