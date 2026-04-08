// AbilityLibrary.cpp

#include "NexusAbilityFunctionLibrary.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/Engine.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "NexusCombatFunctionLibrary.h"
#include "Nexus/Character/NexusCharacterBase.h"
#include "Nexus/Controller/NexusPlayerController.h"
#include "Nexus/GameplayAbilitySystem/Abilities/NexusGameplayAbility.h"
#include "Nexus/GameplayAbilitySystem/AbilitySystemComponent/NexusAbilitySystemComponent.h"

namespace
{
	bool BuildCueContext(
		UAbilitySystemComponent* ASC,
		AActor* TargetActor,
		AActor* InstigatorActor,
		AActor* EffectCauserActor,
		UObject* OptionalSourceObject,
		FGameplayEffectContextHandle& OutEffectContext)
	{
		if (!ASC || !IsValid(TargetActor))
		{
			return false;
		}

		OutEffectContext = ASC->MakeEffectContext();

		AActor* ResolvedInstigator = IsValid(InstigatorActor) ? InstigatorActor : TargetActor;
		AActor* ResolvedEffectCauser = IsValid(EffectCauserActor) ? EffectCauserActor : ResolvedInstigator;

		if (IsValid(ResolvedInstigator))
		{
			OutEffectContext.AddInstigator(ResolvedInstigator, ResolvedEffectCauser);
		}

		if (OptionalSourceObject)
		{
			OutEffectContext.AddSourceObject(OptionalSourceObject);
		}

		return true;
	}

	UWorld* ResolveWorldFromContext(const UObject* WorldContextObject)
	{
		if (!WorldContextObject || !GEngine)
		{
			return nullptr;
		}

		return GEngine->GetWorldFromContextObject(
			WorldContextObject,
			EGetWorldErrorMode::ReturnNull);
	}

	UPrimitiveComponent* GetBestCollisionComponent(const AActor* Actor)
	{
		if (!IsValid(Actor))
		{
			return nullptr;
		}

		if (UPrimitiveComponent* RootPrimitive = Cast<UPrimitiveComponent>(Actor->GetRootComponent()))
		{
			return RootPrimitive;
		}

		return Actor->FindComponentByClass<UPrimitiveComponent>();
	}

	bool TryGetClosestPointOnActor(
		const AActor* TargetActor,
		const FVector& QueryOrigin,
		FVector& OutClosestPoint)
	{
		OutClosestPoint = FVector::ZeroVector;

		UPrimitiveComponent* CollisionComponent = GetBestCollisionComponent(TargetActor);
		if (!CollisionComponent)
		{
			return false;
		}

		const float DistanceToSurface =
			CollisionComponent->GetClosestPointOnCollision(QueryOrigin, OutClosestPoint);

		return DistanceToSurface >= 0.f;
	}
}

UNexusAbilitySystemComponent* UNexusAbilityFunctionLibrary::GetNexusASCFromActor(const AActor* Actor)
{
	if (!IsValid(Actor))
	{
		return nullptr;
	}

	return Cast<UNexusAbilitySystemComponent>(
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(const_cast<AActor*>(Actor)));
}

ANexusPlayerController* UNexusAbilityFunctionLibrary::GetNexusPlayerControllerFromActor(const AActor* Actor)
{
	if (!IsValid(Actor))
	{
		return nullptr;
	}

	if (const APawn* Pawn = Cast<APawn>(Actor))
	{
		return Cast<ANexusPlayerController>(Pawn->GetController());
	}

	return Cast<ANexusPlayerController>(const_cast<AActor*>(Actor));
}

bool UNexusAbilityFunctionLibrary::AbilityRequiresUsableTarget(const UNexusGameplayAbility* Ability)
{
	return IsValid(Ability) && Ability->bRequiresValidTarget;
}

bool UNexusAbilityFunctionLibrary::IsAbilityTargetUsable(
	const UNexusGameplayAbility* Ability,
	const ANexusCharacterBase* SourceCharacter,
	const ANexusCharacterBase* TargetCharacter)
{
	if (!IsValid(Ability))
	{
		return false;
	}

	if (!Ability->bRequiresValidTarget)
	{
		return true;
	}

	if (!UNexusCombatFunctionLibrary::IsValidLivingEnemyTarget(SourceCharacter, TargetCharacter))
	{
		return false;
	}

	if (!Ability->bRequiresTargetInRange)
	{
		return true;
	}

	return UNexusCombatFunctionLibrary::IsWithinRange(
		SourceCharacter,
		TargetCharacter,
		Ability->ActivationRange);
}

bool UNexusAbilityFunctionLibrary::TryGetUsableControllerTargetForAbility(
	const ANexusPlayerController* Controller,
	const ANexusCharacterBase* SourceCharacter,
	const UNexusGameplayAbility* Ability,
	ANexusCharacterBase*& OutTargetCharacter)
{
	OutTargetCharacter = nullptr;

	if (!IsValid(Ability))
	{
		return false;
	}

	if (!Ability->bRequiresValidTarget)
	{
		return true;
	}

	if (!IsValid(Controller) || !IsValid(SourceCharacter))
	{
		return false;
	}

	ANexusCharacterBase* CurrentTarget = Controller->GetCurrentTargetedCharacter();
	if (!IsAbilityTargetUsable(Ability, SourceCharacter, CurrentTarget))
	{
		return false;
	}

	OutTargetCharacter = CurrentTarget;
	return true;
}

bool UNexusAbilityFunctionLibrary::SendTargetedGameplayEventToActor(
	AActor* SourceActor,
	const FGameplayTag& EventTag,
	AActor* TargetActor,
	AActor* OptionalObject)
{
	if (!IsValid(SourceActor) || !EventTag.IsValid())
	{
		return false;
	}

	FGameplayEventData Payload;
	Payload.EventTag = EventTag;
	Payload.Instigator = SourceActor;
	Payload.Target = TargetActor;
	Payload.OptionalObject = OptionalObject ? OptionalObject : TargetActor;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(SourceActor, EventTag, Payload);
	return true;
}

bool UNexusAbilityFunctionLibrary::BuildTargetHitResultFromOrigin(
	const AActor* TargetActor,
	const FVector& Origin,
	FHitResult& OutHitResult)
{
	OutHitResult = FHitResult();

	if (!IsValid(TargetActor))
	{
		return false;
	}

	const UPrimitiveComponent* CollisionComponent = GetBestCollisionComponent(TargetActor);

	FVector ImpactPoint = TargetActor->GetActorLocation();
	if (FVector ClosestPoint = FVector::ZeroVector; TryGetClosestPointOnActor(TargetActor, Origin, ClosestPoint))
	{
		ImpactPoint = ClosestPoint;
	}

	FVector ImpactNormal = (ImpactPoint - Origin).GetSafeNormal();
	if (ImpactNormal.IsNearlyZero())
	{
		ImpactNormal = (TargetActor->GetActorLocation() - Origin).GetSafeNormal();
	}
	if (ImpactNormal.IsNearlyZero())
	{
		ImpactNormal = TargetActor->GetActorForwardVector();
	}
	if (ImpactNormal.IsNearlyZero())
	{
		ImpactNormal = FVector::UpVector;
	}

	OutHitResult.bBlockingHit = true;
	OutHitResult.TraceStart = Origin;
	OutHitResult.TraceEnd = ImpactPoint;
	OutHitResult.Location = ImpactPoint;
	OutHitResult.ImpactPoint = ImpactPoint;
	OutHitResult.Normal = ImpactNormal;
	OutHitResult.ImpactNormal = ImpactNormal;
	OutHitResult.Distance = FVector::Distance(Origin, ImpactPoint);
	OutHitResult.Component = const_cast<UPrimitiveComponent*>(CollisionComponent);

	return true;
}

bool UNexusAbilityFunctionLibrary::HasLineOfSightToTarget(
	const UObject* WorldContextObject,
	const FVector& Origin,
	const AActor* TargetActor,
	ECollisionChannel TraceChannel,
	const AActor* ActorToIgnore,
	const AActor* AdditionalIgnoredActor)
{
	UWorld* World = ResolveWorldFromContext(WorldContextObject);
	if (!World || !IsValid(TargetActor))
	{
		return false;
	}

	FVector TargetPoint = TargetActor->GetActorLocation();
	if (FVector ClosestPoint = FVector::ZeroVector; TryGetClosestPointOnActor(TargetActor, Origin, ClosestPoint))
	{
		TargetPoint = ClosestPoint;
	}

	FHitResult Hit;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(NexusAbilityLineOfSight), false);

	if (IsValid(ActorToIgnore))
	{
		QueryParams.AddIgnoredActor(const_cast<AActor*>(ActorToIgnore));
	}

	if (IsValid(AdditionalIgnoredActor) && AdditionalIgnoredActor != ActorToIgnore)
	{
		QueryParams.AddIgnoredActor(const_cast<AActor*>(AdditionalIgnoredActor));
	}

	const bool bHit = World->LineTraceSingleByChannel(
		Hit,
		Origin,
		TargetPoint,
		TraceChannel,
		QueryParams);

	if (!bHit)
	{
		return true;
	}

	return Hit.GetActor() == TargetActor;
}

bool UNexusAbilityFunctionLibrary::QueryRadialEnemyCharacterTargets(
	const UObject* WorldContextObject,
	const FVector& Origin,
	float Radius,
	const ANexusCharacterBase* SourceCharacter,
	TArray<FNexusAbilityTargetHit>& OutTargetHits,
	bool bRequireLineOfSight,
	ECollisionChannel LineOfSightTraceChannel,
	const AActor* AdditionalIgnoredActor)
{
	OutTargetHits.Reset();

	UWorld* World = ResolveWorldFromContext(WorldContextObject);
	if (!World || Radius <= 0.f || !IsValid(SourceCharacter))
	{
		return false;
	}

	const AActor* SourceActor = Cast<AActor>(SourceCharacter);
	if (!IsValid(SourceActor))
	{
		return false;
	}

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(NexusAbilityRadialTargetQuery), false);
	QueryParams.AddIgnoredActor(const_cast<AActor*>(SourceActor));

	if (IsValid(AdditionalIgnoredActor) && AdditionalIgnoredActor != SourceActor)
	{
		QueryParams.AddIgnoredActor(const_cast<AActor*>(AdditionalIgnoredActor));
	}

	TArray<FOverlapResult> Overlaps;
	const bool bAnyOverlap = World->OverlapMultiByObjectType(
		Overlaps,
		Origin,
		FQuat::Identity,
		ObjectQueryParams,
		FCollisionShape::MakeSphere(Radius),
		QueryParams);

	if (!bAnyOverlap)
	{
		return false;
	}

	TSet<TWeakObjectPtr<AActor>> UniqueActors;

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* TargetActor = Overlap.GetActor();
		if (!IsValid(TargetActor) || TargetActor == SourceActor)
		{
			continue;
		}

		if (UniqueActors.Contains(TargetActor))
		{
			continue;
		}

		ANexusCharacterBase* TargetCharacter = Cast<ANexusCharacterBase>(TargetActor);
		if (!IsValid(TargetCharacter))
		{
			continue;
		}

		if (!UNexusCombatFunctionLibrary::IsValidLivingEnemyTarget(SourceCharacter, TargetCharacter))
		{
			continue;
		}

		if (bRequireLineOfSight &&
			!HasLineOfSightToTarget(
				WorldContextObject,
				Origin,
				TargetActor,
				LineOfSightTraceChannel,
				SourceActor,
				AdditionalIgnoredActor))
		{
			continue;
		}

		FNexusAbilityTargetHit TargetHit;
		TargetHit.TargetActor = TargetActor;

		if (!BuildTargetHitResultFromOrigin(TargetActor, Origin, TargetHit.HitResult))
		{
			TargetHit.HitResult.Location = TargetActor->GetActorLocation();
			TargetHit.HitResult.ImpactPoint = TargetActor->GetActorLocation();
			TargetHit.HitResult.Normal = (TargetActor->GetActorLocation() - Origin).GetSafeNormal();
			TargetHit.HitResult.ImpactNormal = TargetHit.HitResult.Normal;
			TargetHit.HitResult.TraceStart = Origin;
			TargetHit.HitResult.TraceEnd = TargetActor->GetActorLocation();
		}

		OutTargetHits.Add(MoveTemp(TargetHit));
		UniqueActors.Add(TargetActor);
	}

	return OutTargetHits.Num() > 0;
}

bool UNexusAbilityFunctionLibrary::AddGameplayCueToActor(
	AActor* TargetActor,
	const FGameplayTag& CueTag,
	AActor* InstigatorActor,
	AActor* EffectCauserActor,
	UObject* OptionalSourceObject)
{
	if (!IsValid(TargetActor) || !CueTag.IsValid())
	{
		return false;
	}

	UAbilitySystemComponent* ASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);

	if (!ASC)
	{
		return false;
	}

	FGameplayEffectContextHandle EffectContext;
	if (!BuildCueContext(
		ASC,
		TargetActor,
		InstigatorActor,
		EffectCauserActor,
		OptionalSourceObject,
		EffectContext))
	{
		return false;
	}

	ASC->AddGameplayCue(CueTag, EffectContext);
	return true;
}

bool UNexusAbilityFunctionLibrary::RemoveGameplayCueFromActor(
	AActor* TargetActor,
	const FGameplayTag& CueTag)
{
	if (!IsValid(TargetActor) || !CueTag.IsValid())
	{
		return false;
	}

	UAbilitySystemComponent* ASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);

	if (!ASC)
	{
		return false;
	}

	ASC->RemoveGameplayCue(CueTag);
	return true;
}

bool UNexusAbilityFunctionLibrary::ExecuteGameplayCueOnActor(
	AActor* TargetActor,
	const FGameplayTag& CueTag,
	AActor* InstigatorActor,
	AActor* EffectCauserActor,
	UObject* OptionalSourceObject)
{
	if (!IsValid(TargetActor) || !CueTag.IsValid())
	{
		return false;
	}

	UAbilitySystemComponent* ASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);

	if (!ASC)
	{
		return false;
	}

	FGameplayEffectContextHandle EffectContext;
	if (!BuildCueContext(
		ASC,
		TargetActor,
		InstigatorActor,
		EffectCauserActor,
		OptionalSourceObject,
		EffectContext))
	{
		return false;
	}

	ASC->ExecuteGameplayCue(CueTag, EffectContext);
	return true;
}

bool UNexusAbilityFunctionLibrary::ExecuteGameplayCueOnActorWithParams(
	AActor* TargetActor,
	const FGameplayTag& CueTag,
	const FGameplayCueParameters& CueParameters)
{
	if (!IsValid(TargetActor) || !CueTag.IsValid())
	{
		return false;
	}

	UAbilitySystemComponent* ASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);

	if (!ASC)
	{
		return false;
	}

	ASC->ExecuteGameplayCue(CueTag, CueParameters);
	return true;
}