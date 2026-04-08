#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "PlayerAttributeSet.generated.h"

UCLASS()
class NEXUS_API UPlayerAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UPlayerAttributeSet();

	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Player", ReplicatedUsing=OnRep_Stamina)
	FGameplayAttributeData Stamina;
	ATTRIBUTE_ACCESSORS_BASIC(UPlayerAttributeSet, Stamina)

	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Player", ReplicatedUsing=OnRep_MaxStamina)
	FGameplayAttributeData MaxStamina;
	ATTRIBUTE_ACCESSORS_BASIC(UPlayerAttributeSet, MaxStamina)

	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Player", ReplicatedUsing=OnRep_StaminaRegenRate)
	FGameplayAttributeData StaminaRegenRate;
	ATTRIBUTE_ACCESSORS_BASIC(UPlayerAttributeSet, StaminaRegenRate)

	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Player", ReplicatedUsing=OnRep_Armor)
	FGameplayAttributeData Armor;
	ATTRIBUTE_ACCESSORS_BASIC(UPlayerAttributeSet, Armor)

	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Player", ReplicatedUsing=OnRep_MaxArmor)
	FGameplayAttributeData MaxArmor;
	ATTRIBUTE_ACCESSORS_BASIC(UPlayerAttributeSet, MaxArmor)

	UFUNCTION()
	void OnRep_Stamina(const FGameplayAttributeData& OldStamina) const { GAMEPLAYATTRIBUTE_REPNOTIFY(UPlayerAttributeSet, Stamina, OldStamina); }
	UFUNCTION()
	void OnRep_MaxStamina(const FGameplayAttributeData& OldMaxStamina) const { GAMEPLAYATTRIBUTE_REPNOTIFY(UPlayerAttributeSet, MaxStamina, OldMaxStamina); }
	UFUNCTION()
	void OnRep_StaminaRegenRate(const FGameplayAttributeData& OldStaminaRegenRate) const { GAMEPLAYATTRIBUTE_REPNOTIFY(UPlayerAttributeSet, StaminaRegenRate, OldStaminaRegenRate); }
	UFUNCTION()
	void OnRep_Armor(const FGameplayAttributeData& OldArmor) const { GAMEPLAYATTRIBUTE_REPNOTIFY(UPlayerAttributeSet, Armor, OldArmor); }
	UFUNCTION()
	void OnRep_MaxArmor(const FGameplayAttributeData& OldMaxArmor) const { GAMEPLAYATTRIBUTE_REPNOTIFY(UPlayerAttributeSet, MaxArmor, OldMaxArmor); }

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
};
