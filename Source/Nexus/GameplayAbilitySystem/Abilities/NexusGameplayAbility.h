#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayTagContainer.h"
#include "Nexus/NexusEnumTypes.h"
#include "NexusGameplayAbility.generated.h"

class UCostAndCooldownConfig;
class UAbilityInfo;
class ANexusCharacterBase;
class ANexusPlayerCharacter;
class ANexusPlayerController;
class UAbilitySystemComponent;
class UNexusAbilitySystemComponent;
class UGameplayEffect;

USTRUCT(BlueprintType)
struct FAbilityBindingConfig
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Nexus|Tags")
	FGameplayTag InputTag;
};

UCLASS()
class NEXUS_API UNexusGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UNexusGameplayAbility();

	UFUNCTION(BlueprintCallable, Category="Nexus|Ability")
	bool CommitAbilityWithSetByCaller(
		float CooldownDuration = -1.f,
		float CostAmount = -1.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="AbilityConfig")
	TObjectPtr<UAbilityInfo> AbilityInfo;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="AbilityConfig")
	TObjectPtr<UCostAndCooldownConfig> CostAndCooldownConfig;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Activation")
	bool bRequiresValidTarget = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Activation")
	bool bRequiresTargetInRange = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Activation")
	float ActivationRange = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Activation")
	bool bActivateByEvent = false;

public:
	UFUNCTION(BlueprintCallable, Category="UI")
	FText GetAbilityDisplayName() const;

	UFUNCTION(BlueprintCallable, Category="AbilitySystem")
	EAbilityContainerInfo GetContainerToShowIn() const;

	UFUNCTION(BlueprintCallable, Category="Nexus|Ability")
	ANexusCharacterBase* GetNexusCharacterFromActorInfo() const;

	UFUNCTION(BlueprintCallable, Category="Nexus|Ability")
	ANexusPlayerCharacter* GetNexusPlayerCharacterFromActorInfo() const;

	UFUNCTION(BlueprintCallable, Category="Nexus|Ability")
	ANexusPlayerController* GetNexusPlayerControllerFromActorInfo() const;

	UFUNCTION(BlueprintCallable, Category="Nexus|Ability")
	UNexusAbilitySystemComponent* GetNexusAbilitySystemComponentFromActorInfo() const;

	UFUNCTION(BlueprintPure, Category="Nexus|Ability")
	FGameplayTag GetPrimaryActivationEventTag() const;

	UFUNCTION(BlueprintCallable, Category="Nexus|Ability")
	UAbilitySystemComponent* GetSourceAbilitySystemComponent() const;
	
	ANexusCharacterBase* GetTargetCharacterFromEventData(const FGameplayEventData* TriggerEventData) const;

	UFUNCTION(BlueprintCallable, Category="Nexus|Ability")
	bool IsAbilityTargetUsable(const ANexusCharacterBase* SourceCharacter, const ANexusCharacterBase* TargetCharacter) const;
	
	bool IsAbilityTargetUsable(const ANexusCharacterBase* TargetCharacter) const;

	UFUNCTION(BlueprintCallable, Category="Nexus|Ability")
	bool TryGetUsableControllerTarget(ANexusCharacterBase*& OutTargetCharacter) const;

	UFUNCTION(BlueprintPure, Category="Nexus|Ability")
	bool AbilityRequiresUsableTarget() const;

	UFUNCTION(BlueprintPure, Category="Nexus|Ability")
	float GetEffectiveActivationRange() const;

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