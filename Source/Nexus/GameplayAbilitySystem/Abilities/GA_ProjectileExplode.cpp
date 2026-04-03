#include "GA_ProjectileExplode.h"

#include "Abilities/GameplayAbilityTargetTypes.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "Nexus/NexusGameplayTags.h"
#include "Nexus/Character/NexusCharacterBase.h"

UGA_ProjectileExplode::UGA_ProjectileExplode()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

	bActivateByEvent = true;
	bRequiresValidTarget = false;

	const bool bAlreadyHasExplodeTrigger = AbilityTriggers.ContainsByPredicate(
		[](const FAbilityTriggerData& TriggerData)
		{
			return TriggerData.TriggerSource == EGameplayAbilityTriggerSource::GameplayEvent &&
				TriggerData.TriggerTag == NexusGameplayTags::Event_Projectile_Explode;
		});

	if (!bAlreadyHasExplodeTrigger)
	{
		FAbilityTriggerData TriggerData;
		TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
		TriggerData.TriggerTag = NexusGameplayTags::Event_Projectile_Explode;
		AbilityTriggers.Add(TriggerData);
	}
}

void UGA_ProjectileExplode::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!ActorInfo || !ActorInfo->IsNetAuthority())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
		return;
	}

	if (!TriggerEventData)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
		return;
	}
	
	FHitResult Hit;
	if (TryExtractHitResult(*TriggerEventData, Hit))
	{
		const FVector ExplosionOrigin = Hit.ImpactPoint.IsNearlyZero()
			? Hit.Location
			: Hit.ImpactPoint;

		ApplyExplosionAtLocation(ExplosionOrigin, *TriggerEventData, &Hit);
	}
	else
	{
		const AActor* OptionalActor = Cast<AActor>(TriggerEventData->OptionalObject.Get());
		const AActor* InstigatorActor = TriggerEventData->Instigator.Get();
		const AActor* AvatarActor = GetAvatarActorFromActorInfo();

		const FVector FallbackOrigin =
			OptionalActor ? OptionalActor->GetActorLocation() :
			InstigatorActor ? InstigatorActor->GetActorLocation() :
			AvatarActor ? AvatarActor->GetActorLocation() :
			FVector::ZeroVector;

		ApplyExplosionAtLocation(FallbackOrigin, *TriggerEventData, nullptr);
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

bool UGA_ProjectileExplode::TryExtractHitResult(const FGameplayEventData& EventData, FHitResult& OutHit) const
{
	for (int32 Index = 0; Index < EventData.TargetData.Num(); ++Index)
	{
		const FGameplayAbilityTargetData* TargetData = EventData.TargetData.Get(Index);
		if (!TargetData)
		{
			continue;
		}

		const FHitResult* HitResult = TargetData->GetHitResult();
		if (HitResult)
		{
			OutHit = *HitResult;
			return true;
		}
	}

	return false;
}

void UGA_ProjectileExplode::ApplyExplosionAtLocation(
	const FVector& ExplosionOrigin,
	const FGameplayEventData& EventData,
	const FHitResult* DirectHit)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	AActor* SourceActor = const_cast<AActor*>(EventData.Instigator.Get());
	if (!SourceActor)
	{
		SourceActor = GetAvatarActorFromActorInfo();
	}

	const AActor* ProjectileActor = Cast<AActor>(EventData.OptionalObject.Get());

	const float FinalDamage = EventData.EventMagnitude > 0.f
		? EventData.EventMagnitude
		: FallbackDamage;

	if (FinalDamage <= 0.f)
	{
		return;
	}

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(GA_ProjectileExplode), false);
	if (SourceActor)
	{
		QueryParams.AddIgnoredActor(SourceActor);
	}
	if (ProjectileActor)
	{
		QueryParams.AddIgnoredActor(ProjectileActor);
	}

	TArray<FOverlapResult> Overlaps;
	World->OverlapMultiByObjectType(
		Overlaps,
		ExplosionOrigin,
		FQuat::Identity,
		ObjectQueryParams,
		FCollisionShape::MakeSphere(ExplosionRadius),
		QueryParams
	);

	TSet<AActor*> UniqueTargets;

	if (DirectHit)
	{
		if (AActor* DirectHitActor = DirectHit->GetActor())
		{
			if (DirectHitActor != SourceActor)
			{
				UniqueTargets.Add(DirectHitActor);
			}
		}
	}

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* TargetActor = Overlap.GetActor();
		if (!IsValid(TargetActor) || TargetActor == SourceActor)
		{
			continue;
		}

		UniqueTargets.Add(TargetActor);
	}

	for (AActor* TargetActor : UniqueTargets)
	{
		ApplyDamageToActor(TargetActor, SourceActor, FinalDamage);
	}
}

void UGA_ProjectileExplode::ApplyDamageToActor(
	AActor* TargetActor,
	AActor* SourceActor,
	float DamageAmount) const
{
	if (!IsValid(TargetActor) || !IsValid(SourceActor) || DamageAmount <= 0.f)
	{
		return;
	}

	ANexusCharacterBase* SourceCharacter = Cast<ANexusCharacterBase>(SourceActor);
	ANexusCharacterBase* TargetCharacter = Cast<ANexusCharacterBase>(TargetActor);

	if (!SourceCharacter || !TargetCharacter)
	{
		return;
	}

	if (SourceCharacter->GetTeamID() == TargetCharacter->GetTeamID())
	{
		return;
	}

	SourceCharacter->ApplyDamageToTarget(TargetCharacter, DamageAmount);
}