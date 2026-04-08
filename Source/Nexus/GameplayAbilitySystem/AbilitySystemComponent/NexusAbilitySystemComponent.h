#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "NexusAbilitySystemComponent.generated.h"

class UNexusGameplayAbility;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class NEXUS_API UNexusAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	UNexusAbilitySystemComponent();

	void AbilityInputTagPressed(const FGameplayTag& InputTag);
	void AbilityInputTagReleased(const FGameplayTag& InputTag);

	const FGameplayAbilitySpec* FindAbilitySpecByInputTag(const FGameplayTag& InputTag) const;
	const UNexusGameplayAbility* FindNexusAbilityCDOByInputTag(const FGameplayTag& InputTag) const;

protected:
	virtual void BeginPlay() override;
	virtual void OnRep_ActivateAbilities() override;

	bool CanProcessAbilityInput() const;
	void BroadcastOwnerGrantedAbilitiesChanged() const;

protected:
	TArray<FGameplayAbilitySpec> LastActivatedAbilitySpecs;
};