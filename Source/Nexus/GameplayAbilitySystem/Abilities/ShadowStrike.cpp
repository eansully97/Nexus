#include "ShadowStrike.h"


#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Nexus/Character/NexusCharacterBase.h"
#include "Nexus/Controller/NexusPlayerController.h"
#include "Nexus/DataAssets/AbilityInfo.h"

UShadowStrike::UShadowStrike()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UShadowStrike::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	CachedSourceCharacter = GetSourceCharacter();

	if (!IsValid(CachedSourceCharacter))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	
	const AActor* EventTargetActor = TriggerEventData ? TriggerEventData->Target.Get() : nullptr;
	const ANexusCharacterBase* EventTargetCharacter = Cast<ANexusCharacterBase>(EventTargetActor);
	CachedTargetCharacter = const_cast<ANexusCharacterBase*>(EventTargetCharacter);

	if (!IsValidTarget(CachedSourceCharacter, CachedTargetCharacter))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	
	if (!IsValidTarget(CachedSourceCharacter, CachedTargetCharacter))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!IsInRange(CachedSourceCharacter, CachedTargetCharacter))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	FVector TeleportLocation = FVector::ZeroVector;
	FRotator TeleportRotation = FRotator::ZeroRotator;

	if (!FindTeleportLocation(TeleportLocation, TeleportRotation))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	
	ApplyCostSetByCaller();
	TeleportAndFaceTarget(TeleportLocation);

	if (bStopTargetMovement)
	{
		StunTarget();
	}

	if (!ShadowStrikeMontage)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

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
}

void UShadowStrike::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	ApplyCooldownSetByCaller();
	Cleanup();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

ANexusCharacterBase* UShadowStrike::GetSourceCharacter() const
{
	return Cast<ANexusCharacterBase>(GetAvatarActorFromActorInfo());
}

bool UShadowStrike::IsValidTarget(ANexusCharacterBase* SourceCharacter, ANexusCharacterBase* TargetCharacter) const
{
	if (!IsValid(SourceCharacter) || !IsValid(TargetCharacter))
	{
		return false;
	}

	if (SourceCharacter == TargetCharacter)
	{
		return false;
	}

	if (SourceCharacter->GetTeamID() == TargetCharacter->GetTeamID()) return false;
	if (TargetCharacter->GetIsDead()) return false;

	return true;
}

bool UShadowStrike::IsInRange(ANexusCharacterBase* SourceCharacter, ANexusCharacterBase* TargetCharacter) const
{
	if (!IsValid(SourceCharacter) || !IsValid(TargetCharacter))
	{
		return false;
	}

	return FVector::DistSquared(
		SourceCharacter->GetActorLocation(),
		TargetCharacter->GetActorLocation()) <= FMath::Square(MaxRange);
}

bool UShadowStrike::FindTeleportLocation(FVector& OutLocation, FRotator& OutRotation) const
{
	if (!IsValid(CachedTargetCharacter) || !IsValid(GetAvatarActorFromActorInfo()))
	{
		return false;
	}

	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	const FVector MyLocation = AvatarActor->GetActorLocation();
	const FVector TargetLocation = CachedTargetCharacter->GetActorLocation();

	// Direction from me -> target
	FVector ToTarget = TargetLocation - MyLocation;
	ToTarget.Z = 0.f;

	if (ToTarget.IsNearlyZero())
	{
		return false;
	}

	ToTarget.Normalize();

	// This is the "far side" of the target from my current position.
	const FVector OppositeSideDirection = ToTarget;

	// Main desired teleport point
	const FVector BaseLocation =
		TargetLocation
		+ (OppositeSideDirection * TeleportDistanceBeyondTarget)
		+ FVector(0.f, 0.f, UpOffset);

	// Try exact point first, then a few nearby offsets
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

	// You should ideally match these to your character capsule.
	const float CapsuleRadius = 42.f;
	const float CapsuleHalfHeight = 96.f;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(AvatarActor);
	QueryParams.AddIgnoredActor(CachedTargetCharacter);

	for (const FVector& Candidate : CandidateLocations)
	{
		FCollisionShape CapsuleShape = FCollisionShape::MakeCapsule(CapsuleRadius, CapsuleHalfHeight);

		const bool bBlocked = World->OverlapBlockingTestByChannel(
			Candidate,
			FQuat::Identity,
			ECC_Pawn,
			CapsuleShape,
			QueryParams
		);

		if (!bBlocked)
		{
			OutLocation = Candidate;

			// Face toward the target
			OutRotation = UKismetMathLibrary::FindLookAtRotation(OutLocation, TargetLocation);
			OutRotation.Pitch = 0.f;
			OutRotation.Roll = 0.f;

			return true;
		}
	}

	return false;
}

void UShadowStrike::TeleportAndFaceTarget(const FVector& TeleportLocation)
{
	if (!IsValid(CachedSourceCharacter) || !IsValid(CachedTargetCharacter))
	{
		return;
	}

	const FVector SourceLoc = TeleportLocation;
	FVector TargetLoc = CachedTargetCharacter->GetActorLocation();

	// Ignore vertical difference for cleaner melee facing.
	TargetLoc.Z = SourceLoc.Z;

	const FRotator LookAtRot = UKismetMathLibrary::FindLookAtRotation(SourceLoc, TargetLoc);
	const FRotator FlatRot(0.f, LookAtRot.Yaw, 0.f);

	CachedSourceCharacter->TeleportTo(TeleportLocation, FlatRot);

	if (UCharacterMovementComponent* MoveComp = CachedSourceCharacter->GetCharacterMovement())
	{
		MoveComp->StopMovementImmediately();
	}

	CachedSourceCharacter->SetActorRotation(FlatRot);

	if (AController* Controller = CachedSourceCharacter->GetController())
	{
		Controller->SetControlRotation(FlatRot);
	}
}

void UShadowStrike::StunTarget() const
{
	if (!IsValid(CachedTargetCharacter))
	{
		return;
	}
	CachedSourceCharacter->ApplyStunToTarget(CachedTargetCharacter, StunDuration);
}

void UShadowStrike::DamageTarget() const
{
	if (!IsValid(CachedTargetCharacter))
	{
		return;
	}
	CachedSourceCharacter->ApplyDamageToTarget(CachedTargetCharacter, AbilityInfo->Damage);
}

void UShadowStrike::OnMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UShadowStrike::OnMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UShadowStrike::OnMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UShadowStrike::Cleanup()
{
	if (MontageTask)
	{
		MontageTask->EndTask();
		MontageTask = nullptr;
	}

	CachedSourceCharacter = nullptr;
	CachedTargetCharacter = nullptr;
}