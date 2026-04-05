#include "GA_ProjectileExplode.h"

#include "Abilities/GameplayAbilityTargetTypes.h"
#include "Nexus/NexusGameplayTags.h"
#include "Nexus/Character/NexusCharacterBase.h"
#include "Nexus/FunctionLibraries/NexusAbilityFunctionLibrary.h"

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
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

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

	FHitResult DirectHit;
	if (TryExtractHitResult(*TriggerEventData, DirectHit))
	{
		const FVector ExplosionOrigin = DirectHit.ImpactPoint.IsNearlyZero()
			? DirectHit.Location
			: DirectHit.ImpactPoint;

		ApplyExplosionAtLocation(ExplosionOrigin, *TriggerEventData, &DirectHit);
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

bool UGA_ProjectileExplode::TryExtractHitResult(
	const FGameplayEventData& EventData,
	FHitResult& OutHit) const
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
	const AActor* InstigatorActor = EventData.Instigator.Get();

	ANexusCharacterBase* SourceCharacter =
		Cast<ANexusCharacterBase>(const_cast<AActor*>(InstigatorActor));

	if (!SourceCharacter)
	{
		SourceCharacter = Cast<ANexusCharacterBase>(GetAvatarActorFromActorInfo());
	}

	if (!SourceCharacter)
	{
		return;
	}

	const AActor* ProjectileActor = Cast<AActor>(EventData.OptionalObject.Get());

	const float FinalDamage = EventData.EventMagnitude > 0.f
		? EventData.EventMagnitude
		: FallbackDamage;

	if (FinalDamage <= 0.f)
	{
		return;
	}

	TArray<FNexusAbilityTargetHit> TargetHits;
	UNexusAbilityFunctionLibrary::QueryRadialEnemyCharacterTargets(
		this,
		ExplosionOrigin,
		ExplosionRadius,
		SourceCharacter,
		TargetHits,
		false,
		ECC_Visibility,
		ProjectileActor);

	if (DirectHit)
	{
		if (AActor* DirectHitActor = DirectHit->GetActor())
		{
			bool bFoundExistingEntry = false;

			for (FNexusAbilityTargetHit& TargetHit : TargetHits)
			{
				if (TargetHit.TargetActor == DirectHitActor)
				{
					TargetHit.HitResult = *DirectHit;
					bFoundExistingEntry = true;
					break;
				}
			}

			if (!bFoundExistingEntry)
			{
				if (ANexusCharacterBase* DirectHitCharacter = Cast<ANexusCharacterBase>(DirectHitActor))
				{
					if (SourceCharacter->GetTeamID() != DirectHitCharacter->GetTeamID())
					{
						FNexusAbilityTargetHit DirectTargetHit;
						DirectTargetHit.TargetActor = DirectHitActor;
						DirectTargetHit.HitResult = *DirectHit;
						TargetHits.Add(MoveTemp(DirectTargetHit));
					}
				}
			}
		}
	}

	for (const FNexusAbilityTargetHit& TargetHit : TargetHits)
	{
		ApplyDamageToActor(
			TargetHit.TargetActor.Get(),
			SourceCharacter,
			FinalDamage,
			&TargetHit.HitResult);
	}
}

void UGA_ProjectileExplode::ApplyDamageToActor(
	AActor* TargetActor,
	ANexusCharacterBase* SourceCharacter,
	float DamageAmount,
	const FHitResult* HitResult) const
{
	if (!IsValid(TargetActor) || !IsValid(SourceCharacter) || DamageAmount <= 0.f)
	{
		return;
	}

	ANexusCharacterBase* TargetCharacter = Cast<ANexusCharacterBase>(TargetActor);
	if (!TargetCharacter)
	{
		return;
	}

	if (SourceCharacter->GetTeamID() == TargetCharacter->GetTeamID())
	{
		return;
	}

	SourceCharacter->ApplyDamageToTargetWithCueParams(
		TargetCharacter,
		DamageAmount,
		SourceCharacter,
		SourceCharacter,
		HitResult
		);
}