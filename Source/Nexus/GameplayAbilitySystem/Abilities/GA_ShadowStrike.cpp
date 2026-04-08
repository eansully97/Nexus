#include "GA_ShadowStrike.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Nexus/Character/NexusCharacterBase.h"
#include "Nexus/NexusGameplayTags.h"

UGA_ShadowStrike::UGA_ShadowStrike()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	bActivateByEvent = true;
	bRequiresValidTarget = true;
	bRequiresTargetInRange = true;
	ActivationRange = 1200.f;
	ActiveGameplayCueTag = NexusGameplayTags::GameplayCue_Ability_ShadowStrike_Activate;
}

void UGA_ShadowStrike::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	CachedSourceCharacter = GetNexusCharacterFromActorInfo();
	CachedTargetCharacter = GetTargetCharacterFromEventData(TriggerEventData);
	CachedTeleportLocation = FVector::ZeroVector;
	bStrikeExecuted = false;

	if (!IsValid(CachedSourceCharacter) || !ActorInfo || !ActorInfo->AvatarActor.IsValid())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!IsAbilityTargetUsable(CachedSourceCharacter, CachedTargetCharacter))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!ShadowStrikeMontage)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!FindTeleportLocation(CachedTeleportLocation))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbilityWithSetByCaller())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	TriggerActivationGameplayCues();

	MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		NAME_None,
		ShadowStrikeMontage);

	if (!MontageTask)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageCompleted);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageCancelled);
	MontageTask->ReadyForActivation();

	if (TeleportDelay <= 0.f)
	{
		ExecuteShadowStrike();
		return;
	}

	TeleportDelayTask = UAbilityTask_WaitDelay::WaitDelay(this, TeleportDelay);
	if (!TeleportDelayTask)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	TeleportDelayTask->OnFinish.AddDynamic(this, &ThisClass::OnTeleportDelayFinished);
	TeleportDelayTask->ReadyForActivation();
}

void UGA_ShadowStrike::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	ClearActiveGameplayCue();
	Cleanup();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

bool UGA_ShadowStrike::FindTeleportLocation(FVector& OutLocation) const
{
	if (!IsValid(CachedTargetCharacter) || !IsValid(GetAvatarActorFromActorInfo()))
	{
		return false;
	}

	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	const FVector MyLocation = AvatarActor->GetActorLocation();
	const FVector TargetLocation = CachedTargetCharacter->GetActorLocation();

	FVector ToTarget = TargetLocation - MyLocation;
	ToTarget.Z = 0.f;

	if (ToTarget.IsNearlyZero())
	{
		return false;
	}

	ToTarget.Normalize();

	const FVector OppositeSideDirection = ToTarget;

	const FVector BaseLocation =
		TargetLocation
		+ (OppositeSideDirection * TeleportDistanceBeyondTarget)
		+ FVector(0.f, 0.f, UpOffset);

	TArray<FVector> CandidateLocations;
	CandidateLocations.Add(BaseLocation);

	const FVector RightVector = FVector::CrossProduct(FVector::UpVector, OppositeSideDirection).GetSafeNormal();

	const float SideStep = 50.f;
	const float BackStep = 35.f;

	CandidateLocations.Add(BaseLocation + RightVector * SideStep);
	CandidateLocations.Add(BaseLocation - RightVector * SideStep);
	CandidateLocations.Add(BaseLocation - OppositeSideDirection * BackStep);
	CandidateLocations.Add(BaseLocation + RightVector * SideStep - OppositeSideDirection * BackStep);
	CandidateLocations.Add(BaseLocation - RightVector * SideStep - OppositeSideDirection * BackStep);

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	const float CapsuleRadius = 42.f;
	const float CapsuleHalfHeight = 96.f;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(AvatarActor);
	QueryParams.AddIgnoredActor(CachedTargetCharacter);

	for (const FVector& Candidate : CandidateLocations)
	{
		const FCollisionShape CapsuleShape = FCollisionShape::MakeCapsule(CapsuleRadius, CapsuleHalfHeight);

		const bool bBlocked = World->OverlapBlockingTestByChannel(
			Candidate,
			FQuat::Identity,
			ECC_Pawn,
			CapsuleShape,
			QueryParams);

		if (!bBlocked)
		{
			OutLocation = Candidate;
			return true;
		}
	}

	return false;
}

void UGA_ShadowStrike::TeleportAndFaceTarget(const FVector& TeleportLocation)
{
	if (!IsValid(CachedSourceCharacter) || !IsValid(CachedTargetCharacter))
	{
		return;
	}

	const FVector SourceLoc = TeleportLocation;
	const FVector TargetLoc = CachedTargetCharacter->GetActorLocation();

	const FRotator LookAtRot = UKismetMathLibrary::FindLookAtRotation(SourceLoc, TargetLoc);
	const FRotator YawOnlyRot(0.f, LookAtRot.Yaw, 0.f);

	CachedSourceCharacter->TeleportTo(TeleportLocation, YawOnlyRot);

	if (UCharacterMovementComponent* MoveComp = CachedSourceCharacter->GetCharacterMovement())
	{
		MoveComp->StopActiveMovement();
	}

	CachedSourceCharacter->SetActorRotation(YawOnlyRot);

	if (AController* Controller = CachedSourceCharacter->GetController())
	{
		Controller->SetControlRotation(LookAtRot);
	}
}

void UGA_ShadowStrike::StunTarget() const
{
	if (!IsValid(CachedSourceCharacter) || !IsValid(CachedTargetCharacter))
	{
		return;
	}

	CachedSourceCharacter->ApplyStunToTarget(CachedTargetCharacter, StunDuration);
}

void UGA_ShadowStrike::DamageTarget() const
{
	if (!IsValid(CachedSourceCharacter) || !IsValid(CachedTargetCharacter))
	{
		return;
	}

	CachedSourceCharacter->ApplyDamageToTarget(CachedTargetCharacter, Damage);
}

void UGA_ShadowStrike::ExecuteShadowStrike()
{
	if (bStrikeExecuted)
	{
		return;
	}

	if (!IsValid(CachedSourceCharacter) || !IsValid(CachedTargetCharacter))
	{
		ClearActiveGameplayCue();
		return;
	}

	if (CachedTargetCharacter->GetIsDead())
	{
		ClearActiveGameplayCue();
		return;
	}

	FVector TeleportLocation = CachedTeleportLocation;
	const bool bFoundFreshTeleportLocation = FindTeleportLocation(TeleportLocation);

	if (!bFoundFreshTeleportLocation && TeleportLocation.IsNearlyZero())
	{
		ClearActiveGameplayCue();
		return;
	}

	TeleportAndFaceTarget(TeleportLocation);

	if (bStopTargetMovement)
	{
		StunTarget();
	}

	DamageTarget();

	bStrikeExecuted = true;
	ClearActiveGameplayCue();
}

void UGA_ShadowStrike::OnTeleportDelayFinished()
{
	ExecuteShadowStrike();
}

void UGA_ShadowStrike::OnMontageCompleted()
{
	if (!bStrikeExecuted)
	{
		ExecuteShadowStrike();
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_ShadowStrike::OnMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UGA_ShadowStrike::OnMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UGA_ShadowStrike::Cleanup()
{
	if (TeleportDelayTask)
	{
		TeleportDelayTask->EndTask();
		TeleportDelayTask = nullptr;
	}

	if (MontageTask)
	{
		MontageTask->EndTask();
		MontageTask = nullptr;
	}

	CachedSourceCharacter = nullptr;
	CachedTargetCharacter = nullptr;
	CachedTeleportLocation = FVector::ZeroVector;
	bStrikeExecuted = false;
}
