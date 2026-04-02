#pragma once

#include "CoreMinimal.h"
#include "NexusGameplayAbility.h"
#include "GA_Parry.generated.h"

class ANexusCharacterBase;

UCLASS()
class NEXUS_API UGA_Parry : public UNexusGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Parry();

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	bool TryExtractParryActors(
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayEventData* TriggerEventData,
		ANexusCharacterBase*& OutDefender,
		ANexusCharacterBase*& OutAttacker) const;

	void HandleSuccessfulParry(
		ANexusCharacterBase* Defender,
		ANexusCharacterBase* Attacker,
		const FGameplayEventData* TriggerEventData) const;

	UFUNCTION(BlueprintImplementableEvent, Category="Parry")
	void BP_OnParryActivated(const FGameplayEventData& EventData);

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Parry")
	float StunDuration = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Parry")
	bool bPlayParryCueOnDefender = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Parry")
	bool bPlayParryCueOnAttacker = false;
};