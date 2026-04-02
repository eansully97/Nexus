#include "GA_ShadowStrike.h"


#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Nexus/Character/NexusCharacterBase.h"

UGA_ShadowStrike::UGA_ShadowStrike()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UGA_ShadowStrike::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{

	CachedSourceCharacter = GetSourceCharacter();
	bCommittedSuccessfully = false;

	if (!IsValid(CachedSourceCharacter) || !ActorInfo || !ActorInfo->AvatarActor.IsValid())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const AActor* EventTargetActor = TriggerEventData ? TriggerEventData->Target.Get() : nullptr;
	CachedTargetCharacter = Cast<ANexusCharacterBase>(const_cast<AActor*>(EventTargetActor));

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

	if (!ShadowStrikeMontage)
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

	if (!CommitAbilityWithSetByCaller())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	bCommittedSuccessfully = true;

	TeleportAndFaceTarget(TeleportLocation);

	if (bStopTargetMovement)
	{
		StunTarget();
	}

	DamageTarget();

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

void UGA_ShadowStrike::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	Cleanup();
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

ANexusCharacterBase* UGA_ShadowStrike::GetSourceCharacter() const
{
	return Cast<ANexusCharacterBase>(GetAvatarActorFromActorInfo());
}

bool UGA_ShadowStrike::IsValidTarget(ANexusCharacterBase* SourceCharacter, ANexusCharacterBase* TargetCharacter) const
{
	if (!IsValid(SourceCharacter) || !IsValid(TargetCharacter))
	{
		return false;
	}

	if (SourceCharacter == TargetCharacter)
	{
		return false;
	}

	if (SourceCharacter->GetTeamID() == TargetCharacter->GetTeamID())
	{
		return false;
	}

	if (TargetCharacter->GetIsDead())
	{
		return false;
	}

	return true;
}

bool UGA_ShadowStrike::IsInRange(ANexusCharacterBase* SourceCharacter, ANexusCharacterBase* TargetCharacter) const
{
	if (!IsValid(SourceCharacter) || !IsValid(TargetCharacter))
	{
		return false;
	}

	return FVector::DistSquared(
		SourceCharacter->GetActorLocation(),
		TargetCharacter->GetActorLocation()) <= FMath::Square(MaxRange);
}

bool UGA_ShadowStrike::FindTeleportLocation(FVector& OutLocation, FRotator& OutRotation) const
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
			QueryParams
		);

		if (!bBlocked)
		{
			OutLocation = Candidate;
			OutRotation = UKismetMathLibrary::FindLookAtRotation(OutLocation, TargetLocation);
			OutRotation.Pitch = 0.f;
			OutRotation.Roll = 0.f;
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
	FVector TargetLoc = CachedTargetCharacter->GetActorLocation();
	TargetLoc.Z = SourceLoc.Z;

	const FRotator LookAtRot = UKismetMathLibrary::FindLookAtRotation(SourceLoc, TargetLoc);
	const FRotator FlatRot(0.f, LookAtRot.Yaw, 0.f);

	CachedSourceCharacter->TeleportTo(TeleportLocation, FlatRot);

	if (UCharacterMovementComponent* MoveComp = CachedSourceCharacter->GetCharacterMovement())
	{
		MoveComp->StopActiveMovement();
	}

	CachedSourceCharacter->SetActorRotation(FlatRot);

	if (AController* Controller = CachedSourceCharacter->GetController())
	{
		Controller->SetControlRotation(FlatRot);
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

void UGA_ShadowStrike::OnMontageCompleted()
{
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
	if (MontageTask)
	{
		MontageTask->EndTask();
		MontageTask = nullptr;
	}

	CachedSourceCharacter = nullptr;
	CachedTargetCharacter = nullptr;
	bCommittedSuccessfully = false;
}