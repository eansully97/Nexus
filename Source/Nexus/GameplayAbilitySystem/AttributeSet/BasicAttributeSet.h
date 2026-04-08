// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "BasicAttributeSet.generated.h"

/**
 * 
 */
UCLASS()
class NEXUS_API UBasicAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UBasicAttributeSet();

	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing=OnRep_Health)
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS_BASIC(UBasicAttributeSet, Health)

	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing=OnRep_MaxHealth)
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS_BASIC(UBasicAttributeSet, MaxHealth)

	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Movement", ReplicatedUsing=OnRep_MoveSpeed)
	FGameplayAttributeData MoveSpeed;
	ATTRIBUTE_ACCESSORS_BASIC(UBasicAttributeSet, MoveSpeed)

	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Movement", ReplicatedUsing=OnRep_JumpVelocity)
	FGameplayAttributeData JumpVelocity;
	ATTRIBUTE_ACCESSORS_BASIC(UBasicAttributeSet, JumpVelocity)

	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Movement", ReplicatedUsing=OnRep_AirControl)
	FGameplayAttributeData AirControl;
	ATTRIBUTE_ACCESSORS_BASIC(UBasicAttributeSet, AirControl)

	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldHealth) const { GAMEPLAYATTRIBUTE_REPNOTIFY(UBasicAttributeSet, Health, OldHealth); }
	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth) const { GAMEPLAYATTRIBUTE_REPNOTIFY(UBasicAttributeSet, MaxHealth, OldMaxHealth); }
	UFUNCTION()
	void OnRep_MoveSpeed(const FGameplayAttributeData& OldMoveSpeed) const { GAMEPLAYATTRIBUTE_REPNOTIFY(UBasicAttributeSet, MoveSpeed, OldMoveSpeed); }
	UFUNCTION()
	void OnRep_JumpVelocity(const FGameplayAttributeData& OldJumpVelocity) const { GAMEPLAYATTRIBUTE_REPNOTIFY(UBasicAttributeSet, JumpVelocity, OldJumpVelocity); }
	UFUNCTION()
	void OnRep_AirControl(const FGameplayAttributeData& OldAirControl) const { GAMEPLAYATTRIBUTE_REPNOTIFY(UBasicAttributeSet, AirControl, OldAirControl); }

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
};
