#include "GA_Repulse.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Animation/AnimMontage.h"
#include "Components/SceneComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "Nexus/NexusGameplayTags.h"
#include "Nexus/Character/NexusCharacterBase.h"
#include "Nexus/Character/Player/NexusPlayerCharacter.h"
#include "Nexus/Components/NexusWeaponsManager.h"
#include "Nexus/FunctionLibraries/NexusAbilityFunctionLibrary.h"
#include "Nexus/Weapons/NexusWeaponBase.h"

DEFINE_LOG_CATEGORY_STATIC(LogGARepulse, Log, All);

UGA_Repulse::UGA_Repulse()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	ExecutionEventTag = FGameplayTag();
}

bool UGA_Repulse::ExecuteTriggeredAction()
{
	return ExecuteRepulseBlast();
}

bool UGA_Repulse::ExecuteRepulseBlast()
{
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	if (!ActorInfo || !ActorInfo->IsNetAuthority())
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogGARepulse, Warning, TEXT("Repulse: World is null."));
		return false;
	}

	ANexusCharacterBase* SourceCharacter = GetNexusCharacterFromActorInfo();
	if (!IsValid(SourceCharacter))
	{
		UE_LOG(LogGARepulse, Warning, TEXT("Repulse: SourceCharacter is invalid."));
		return false;
	}

	FVector BlastOrigin = FVector::ZeroVector;
	FVector BlastForwardDirection = FVector::ForwardVector;
	if (!GetRepulseOriginData(BlastOrigin, BlastForwardDirection))
	{
		UE_LOG(LogGARepulse, Warning, TEXT("Repulse: Failed to resolve blast origin data."));
		return false;
	}

	const ANexusPlayerCharacter* PlayerCharacter = GetNexusPlayerCharacterFromActorInfo();
	const UNexusWeaponsManager* WeaponsManager = PlayerCharacter ? PlayerCharacter->GetWeaponsManager() : nullptr;
	ANexusWeaponBase* EquippedWeapon = WeaponsManager ? WeaponsManager->EquippedWeapon : nullptr;

	if (RepulseMuzzleShockwaveCueTag.IsValid())
	{
		FGameplayCueParameters CueParameters;
		CueParameters.Location = BlastOrigin;
		CueParameters.Normal = BlastForwardDirection.GetSafeNormal();
		CueParameters.Instigator = SourceCharacter;
		CueParameters.EffectCauser = EquippedWeapon ? static_cast<AActor*>(EquippedWeapon) : static_cast<AActor*>(SourceCharacter);
		CueParameters.SourceObject = EquippedWeapon ? static_cast<UObject*>(EquippedWeapon) : static_cast<UObject*>(this);

		UNexusAbilityFunctionLibrary::ExecuteGameplayCueOnActorWithParams(
			SourceCharacter,
			RepulseMuzzleShockwaveCueTag,
			CueParameters);
	}

	UNexusAbilityFunctionLibrary::ExecuteGameplayCueOnActor(
		SourceCharacter,
		NexusGameplayTags::GameplayCue_Ability_Repulse_Activate,
		SourceCharacter,
		EquippedWeapon,
		EquippedWeapon);

	DrawRepulseDebug(BlastOrigin, BlastForwardDirection);

	FVector FlatForward = BlastForwardDirection;
	FlatForward.Z = 0.f;
	FlatForward = FlatForward.GetSafeNormal();

	if (FlatForward.IsNearlyZero())
	{
		FlatForward = SourceCharacter->GetActorForwardVector();
		FlatForward.Z = 0.f;
		FlatForward = FlatForward.GetSafeNormal();
	}

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(RepulseOverlap), false, SourceCharacter);
	QueryParams.AddIgnoredActor(SourceCharacter);

	TArray<FOverlapResult> OverlapResults;
	World->OverlapMultiByObjectType(
		OverlapResults,
		BlastOrigin,
		FQuat::Identity,
		ObjectQueryParams,
		FCollisionShape::MakeSphere(BlastRadius),
		QueryParams);

	int32 NumAffectedTargets = 0;

	// Prevent duplicate hits when multiple components from the same actor overlap.
	TSet<TWeakObjectPtr<ANexusCharacterBase>> ProcessedTargets;

	for (const FOverlapResult& Overlap : OverlapResults)
	{
		ANexusCharacterBase* TargetAsCharacter = Cast<ANexusCharacterBase>(Overlap.GetActor());
		if (!IsValid(TargetAsCharacter) || TargetAsCharacter == SourceCharacter)
		{
			continue;
		}

		if (ProcessedTargets.Contains(TargetAsCharacter))
		{
			continue;
		}
		ProcessedTargets.Add(TargetAsCharacter);

		if (!SourceCharacter->IsEnemyTo(TargetAsCharacter))
		{
			continue;
		}

		const FVector TargetLocation = TargetAsCharacter->GetActorLocation();

		FVector ToTarget = TargetLocation - BlastOrigin;
		ToTarget.Z = 0.f;
		ToTarget = ToTarget.GetSafeNormal();

		if (ToTarget.IsNearlyZero())
		{
			continue;
		}

		const float ForwardDot = FVector::DotProduct(FlatForward, ToTarget);
		if (ForwardDot < MinForwardDot)
		{
			continue;
		}

		const float DistanceToTarget = FVector::Distance(BlastOrigin, TargetLocation);
		float StrengthMultiplier = 1.0f;

		if (bScaleKnockbackByDistance && BlastRadius > KINDA_SMALL_NUMBER)
		{
			const float DistanceAlpha = FMath::Clamp(DistanceToTarget / BlastRadius, 0.f, 1.f);
			StrengthMultiplier = FMath::Lerp(1.0f, MinimumDistanceStrengthMultiplier, DistanceAlpha);
		}

		FVector LaunchDirection = ToTarget;
		if (LaunchDirection.IsNearlyZero())
		{
			LaunchDirection = FlatForward;
		}

		const FVector LaunchVelocity =
			(LaunchDirection * KnockbackStrength * StrengthMultiplier) +
			(FVector::UpVector * KnockbackUpwardStrength * StrengthMultiplier);

		if (UCharacterMovementComponent* MoveComp = TargetAsCharacter->GetCharacterMovement())
		{
			MoveComp->StopMovementImmediately();
		}

		// Damage should come FROM the source character TO the target.
		SourceCharacter->ApplyDamageToTarget(TargetAsCharacter, Damage);
		TargetAsCharacter->LaunchCharacter(LaunchVelocity, true, true);

		++NumAffectedTargets;
	}

	UE_LOG(
		LogGARepulse,
		Verbose,
		TEXT("Repulse: Executed at %s. AffectedTargets=%d RawOverlaps=%d"),
		*BlastOrigin.ToString(),
		NumAffectedTargets,
		OverlapResults.Num());

	return true;
}

bool UGA_Repulse::GetRepulseOriginData(
	FVector& OutOrigin,
	FVector& OutForwardDirection) const
{
	OutOrigin = FVector::ZeroVector;
	OutForwardDirection = FVector::ForwardVector;

	const ANexusPlayerCharacter* Character = GetNexusPlayerCharacterFromActorInfo();
	if (!IsValid(Character))
	{
		return false;
	}

	OutOrigin = Character->GetActorLocation() + FVector(0.f, 0.f, 50.f);

	const UNexusWeaponsManager* WeaponsManager = Character->GetWeaponsManager();
	const ANexusWeaponBase* EquippedWeapon = WeaponsManager ? WeaponsManager->EquippedWeapon : nullptr;
	const USceneComponent* WeaponMesh = EquippedWeapon ? EquippedWeapon->GetWeaponMesh() : nullptr;

	if (IsValid(WeaponMesh))
	{
		if (MuzzleSocketName != NAME_None && WeaponMesh->DoesSocketExist(MuzzleSocketName))
		{
			OutOrigin = WeaponMesh->GetSocketLocation(MuzzleSocketName);
		}
		else
		{
			OutOrigin = WeaponMesh->GetComponentLocation();
		}
	}

	FVector AimLocation = FVector::ZeroVector;
	if (!GetAimHitLocation(AimLocation))
	{
		const AController* Controller = Character->GetController();
		const FRotator FallbackRotation = IsValid(Controller)
			? Controller->GetControlRotation()
			: Character->GetActorRotation();

		AimLocation = OutOrigin + (FallbackRotation.Vector() * TraceDistance);
	}

	OutForwardDirection = (AimLocation - OutOrigin).GetSafeNormal();

	if (bFlattenBlastDirectionToPlane)
	{
		OutForwardDirection.Z = 0.f;
		OutForwardDirection = OutForwardDirection.GetSafeNormal();
	}

	if (OutForwardDirection.IsNearlyZero())
	{
		OutForwardDirection = Character->GetActorForwardVector();

		if (bFlattenBlastDirectionToPlane)
		{
			OutForwardDirection.Z = 0.f;
			OutForwardDirection = OutForwardDirection.GetSafeNormal();
		}
	}

	if (OutForwardDirection.IsNearlyZero())
	{
		OutForwardDirection = FVector::ForwardVector;
	}

	OutOrigin += OutForwardDirection * BlastForwardOffset;
	return true;
}

void UGA_Repulse::DrawRepulseDebug(
	const FVector& Origin,
	const FVector& ForwardDirection) const
{
	if (!bDrawDebugRepulse)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FVector SafeForward = ForwardDirection.GetSafeNormal();
	if (SafeForward.IsNearlyZero())
	{
		return;
	}

	// Always show the raw overlap radius.
	DrawDebugSphere(
		World,
		Origin,
		BlastRadius,
		24,
		FColor::Cyan,
		false,
		DebugDrawTime,
		0,
		1.5f);

	// Show the facing line.
	DrawDebugLine(
		World,
		Origin,
		Origin + (SafeForward * BlastRadius),
		FColor::Blue,
		false,
		DebugDrawTime,
		0,
		2.0f);

	// MinForwardDot <= -1 means effectively no directional filtering.
	if (MinForwardDot <= -1.f)
	{
		return;
	}

	const float HalfAngleRadians = FMath::Acos(FMath::Clamp(MinForwardDot, -1.f, 1.f));

	DrawDebugCone(
		World,
		Origin,
		SafeForward * BlastRadius,
		BlastRadius,
		HalfAngleRadians,
		HalfAngleRadians,
		20,
		FColor::Purple,
		false,
		DebugDrawTime,
		0,
		1.5f);
}

bool UGA_Repulse::GetAimHitLocation(FVector& OutHitLocation) const
{
	OutHitLocation = FVector::ZeroVector;

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	APawn* Pawn = Cast<APawn>(AvatarActor);
	if (!IsValid(Pawn))
	{
		return false;
	}

	AController* Controller = Pawn->GetController();
	if (!IsValid(Controller))
	{
		return false;
	}

	FVector ViewLocation = FVector::ZeroVector;
	FRotator ViewRotation = FRotator::ZeroRotator;
	Controller->GetPlayerViewPoint(ViewLocation, ViewRotation);

	const FVector TraceStart = ViewLocation;
	const FVector TraceEnd = TraceStart + (ViewRotation.Vector() * TraceDistance);

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(RepulseAimTrace), false, Pawn);
	Params.AddIgnoredActor(Pawn);

	if (IsValid(AvatarActor))
	{
		Params.AddIgnoredActor(AvatarActor);
	}
	
	const bool bHit = World->LineTraceSingleByChannel(
		Hit,
		TraceStart,
		TraceEnd,
		ECC_Visibility,
		Params);

	OutHitLocation = bHit ? Hit.ImpactPoint : TraceEnd;
	return true;
}