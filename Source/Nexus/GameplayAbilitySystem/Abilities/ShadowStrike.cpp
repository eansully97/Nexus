#include "ShadowStrike.h"


#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Nexus/Character/NexusCharacterBase.h"
#include "Nexus/Controller/NexusPlayerController.h"

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

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
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
	if (!IsValid(CachedTargetCharacter))
	{
		return false;
	}

	const FVector TargetLocation = CachedTargetCharacter->GetActorLocation();
	const FVector TargetForward = CachedTargetCharacter->GetActorForwardVector();
	const FVector TargetRight = CachedTargetCharacter->GetActorRightVector();

	OutLocation = TargetLocation
		- (TargetForward * TeleportDistanceBehindTarget)
		+ (TargetRight * SideOffset);

	OutRotation = UKismetMathLibrary::FindLookAtRotation(OutLocation, TargetLocation);
	OutRotation.Pitch = 0.f;
	OutRotation.Roll = 0.f;

	return true;
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
	FGameplayEventData Payload;
	Payload.Instigator = CachedSourceCharacter;
	Payload.Target = CachedTargetCharacter;
	Payload.EventMagnitude = 0.6f;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(CachedTargetCharacter, FGameplayTag::RequestGameplayTag(FName("Event.Status.Stunned")), Payload);
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