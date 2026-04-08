#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Nexus/GameplayAbilitySystem/Abilities/NexusGameplayAbility.h"
#include "GA_Dash.generated.h"

class UAbilityTask_ApplyRootMotionConstantForce;
class UAbilityTask_WaitDelay;
class ANexusCharacterBase;

UCLASS()
class NEXUS_API UGA_Dash : public UNexusGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Dash();

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

protected:
	UFUNCTION()
	void OnDashDurationFinished();

	FVector GetDashDirection() const;
	void CleanupTasks();

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Dash")
	float DashStrength = 2000.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Dash")
	float DashDuration = 0.3f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Dash")
	bool bIsAdditive = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Dash")
	bool bEnableGravity = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Dash")
	bool bStopMovementOnDashEnd = true;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_ApplyRootMotionConstantForce> RootMotionTask = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_WaitDelay> WaitDelayTask = nullptr;

	UPROPERTY(Transient)
	bool bHasAppliedCooldown = false;
};
