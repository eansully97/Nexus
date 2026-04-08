#include "GA_Dash.h"

#include "Abilities/Tasks/AbilityTask_ApplyRootMotionConstantForce.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Nexus/Character/NexusCharacterBase.h"
#include "Nexus/NexusGameplayTags.h"

UGA_Dash::UGA_Dash()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	ActiveGameplayCueTag = NexusGameplayTags::GameplayCue_Ability_Dash_Activate;
}

void UGA_Dash::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	ANexusCharacterBase* Character = GetNexusCharacterFromActorInfo();
	if (!IsValid(Character) || !ActorInfo || !ActorInfo->AvatarActor.IsValid())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const FVector DashDirection = GetDashDirection();
	if (DashDirection.IsNearlyZero())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	bHasAppliedCooldown = false;

	// Preserve the old Blueprint behavior:
	// spend cost when dash starts, apply cooldown when dash ends.
	ApplyCostSetByCaller();
	TriggerActivationGameplayCues();

	RootMotionTask = UAbilityTask_ApplyRootMotionConstantForce::ApplyRootMotionConstantForce(
		this,
		NAME_None,
		DashDirection,
		DashStrength,
		DashDuration,
		bIsAdditive,
		nullptr,
		ERootMotionFinishVelocityMode::ClampVelocity,
		FVector::ZeroVector,
		0.f,
		bEnableGravity);

	if (!RootMotionTask)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	RootMotionTask->ReadyForActivation();

	// Do not rely on RootMotionTask->OnFinish.
	// Use a delay as the authoritative ability lifetime controller.
	WaitDelayTask = UAbilityTask_WaitDelay::WaitDelay(this, DashDuration);
	if (!WaitDelayTask)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	WaitDelayTask->OnFinish.AddDynamic(this, &ThisClass::OnDashDurationFinished);
	WaitDelayTask->ReadyForActivation();
}

void UGA_Dash::OnDashDurationFinished()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

FVector UGA_Dash::GetDashDirection() const
{
	const ANexusCharacterBase* Character = GetNexusCharacterFromActorInfo();
	if (!IsValid(Character))
	{
		return FVector::ZeroVector;
	}

	FVector InputDirection = FVector::ZeroVector;

	if (const UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement())
	{
		InputDirection = MoveComp->GetLastInputVector();
	}

	InputDirection.Z = 0.f;

	if (InputDirection.IsNearlyZero())
	{
		InputDirection = Character->GetActorForwardVector();
		InputDirection.Z = 0.f;
	}

	return InputDirection.GetSafeNormal();
}

void UGA_Dash::CleanupTasks()
{
	if (WaitDelayTask)
	{
		WaitDelayTask->EndTask();
		WaitDelayTask = nullptr;
	}

	if (RootMotionTask)
	{
		RootMotionTask->EndTask();
		RootMotionTask = nullptr;
	}
}

void UGA_Dash::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (!bHasAppliedCooldown)
	{
		ApplyCooldownSetByCaller();
		bHasAppliedCooldown = true;
	}

	if (bStopMovementOnDashEnd)
	{
		if (ANexusCharacterBase* Character = GetNexusCharacterFromActorInfo())
		{
			if (UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement())
			{
				MoveComp->StopMovementImmediately();
			}
		}
	}

	CleanupTasks();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
