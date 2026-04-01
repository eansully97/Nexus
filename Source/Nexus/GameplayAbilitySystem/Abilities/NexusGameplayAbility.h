#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayTagContainer.h"
#include "Nexus/NexusEnumTypes.h"
#include "NexusGameplayAbility.generated.h"

class UCostAndCooldownConfig;
class UAbilityInfo;
class UNexusAbilityConfig;
class ANexusCharacterBase;
class UAbilitySystemComponent;
class UGameplayEffect;

USTRUCT(BlueprintType)
struct FAbilityTagConfig
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Nexus|Tags")
	FGameplayTag AbilityTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Nexus|Tags")
	FGameplayTag ActivationEventTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Nexus|Tags")
	FGameplayTag InputTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Nexus|Tags")
	FGameplayTag CooldownTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Nexus|Tags")
	FGameplayTag WeaponTag;
};

UCLASS()
class NEXUS_API UNexusGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UNexusGameplayAbility();
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="AbilityConfig")
	FAbilityTagConfig AbilityTagConfig;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="AbilityConfig")
	TObjectPtr<UAbilityInfo> AbilityInfo;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="AbilityConfig")
	TObjectPtr<UCostAndCooldownConfig> CostAndCooldownConfig; 

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Activation")
	bool bRequiresValidTarget = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Activation")
	bool bActivateByEvent = false;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float Damage = 1.f;
	
public:
	UFUNCTION(BlueprintCallable, Category="UI")
	FText GetAbilityDisplayName() const;

	UFUNCTION(BlueprintCallable, Category="AbilitySystem")
	EAbilityContainerInfo GetContainerToShowIn() const;

	UFUNCTION(BlueprintCallable, Category="Nexus|Ability")
	ANexusCharacterBase* GetNexusCharacterFromActorInfo() const;

	UFUNCTION(BlueprintCallable, Category="Nexus|Ability")
	UAbilitySystemComponent* GetSourceAbilitySystemComponent() const;

	UFUNCTION(BlueprintCallable, Category="Nexus|Ability")
	void AddActivationOwnedTagsToSource() const;

	UFUNCTION(BlueprintCallable, Category="Nexus|Ability")
	void RemoveActivationOwnedTagsFromSource() const;

	UFUNCTION(BlueprintCallable, Category="Nexus|Ability")
	void ApplyCooldownSetByCaller(float InDuration = -1.f);

	UFUNCTION(BlueprintCallable, Category="Nexus|Ability")
	void ApplyCostSetByCaller(float InCostAmount = -1.f);

protected:
	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

	bool ApplySetByCallerEffectToOwner(
		TSubclassOf<UGameplayEffect> EffectClass,
		const FGameplayTag& DataTag,
		float Magnitude) const;
};