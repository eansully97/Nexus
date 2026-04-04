#include "NexusCombatFunctionLibrary.h"

#include "AbilitySystemComponent.h"
#include "GameFramework/Actor.h"
#include "Nexus/Character/NexusCharacterBase.h"

ANexusCharacterBase* UNexusCombatFunctionLibrary::GetNexusCharacterFromActor(const AActor* Actor)
{
	if (!IsValid(Actor))
	{
		return nullptr;
	}

	if (const ANexusCharacterBase* DirectCharacter = Cast<ANexusCharacterBase>(Actor))
	{
		return const_cast<ANexusCharacterBase*>(DirectCharacter);
	}

	if (const AActor* OwnerActor = Actor->GetOwner())
	{
		return Cast<ANexusCharacterBase>(const_cast<AActor*>(OwnerActor));
	}

	return nullptr;
}

bool UNexusCombatFunctionLibrary::IsCharacterAlive(const ANexusCharacterBase* Character)
{
	return IsValid(Character) && !Character->GetIsDead();
}

bool UNexusCombatFunctionLibrary::IsValidLivingEnemyTarget(
	const ANexusCharacterBase* SourceCharacter,
	const ANexusCharacterBase* TargetCharacter)
{
	if (!IsValid(SourceCharacter) || !IsValid(TargetCharacter))
	{
		return false;
	}

	if (SourceCharacter == TargetCharacter)
	{
		return false;
	}

	if (TargetCharacter->GetIsDead())
	{
		return false;
	}

	return SourceCharacter->GetTeamID() != TargetCharacter->GetTeamID();
}

bool UNexusCombatFunctionLibrary::IsWithinRange(
	const AActor* SourceActor,
	const AActor* TargetActor,
	float Range)
{
	if (!IsValid(SourceActor) || !IsValid(TargetActor))
	{
		return false;
	}

	if (Range <= 0.f)
	{
		return true;
	}

	return FVector::DistSquared(
		SourceActor->GetActorLocation(),
		TargetActor->GetActorLocation()) <= FMath::Square(Range);
}

bool UNexusCombatFunctionLibrary::IsWithinFacingAngle(
	const AActor* DefenderActor,
	const AActor* AttackerActor,
	float MinDotThreshold)
{
	if (!IsValid(DefenderActor) || !IsValid(AttackerActor))
	{
		return false;
	}

	const FVector DefenderForward = DefenderActor->GetActorForwardVector().GetSafeNormal();
	const FVector ToAttacker =
		(AttackerActor->GetActorLocation() - DefenderActor->GetActorLocation()).GetSafeNormal();

	const float Dot = FVector::DotProduct(DefenderForward, ToAttacker);
	return Dot >= MinDotThreshold;
}

bool UNexusCombatFunctionLibrary::CanCharacterDeflectMeleeHit(
	const ANexusCharacterBase* DefenderCharacter,
	const ANexusCharacterBase* AttackerCharacter,
	float MinDotThreshold,
	const FGameplayTag& DeflectTag)
{
	if (!IsValid(DefenderCharacter) || !IsValid(AttackerCharacter))
	{
		return false;
	}

	if (DefenderCharacter->GetIsDead())
	{
		return false;
	}

	UAbilitySystemComponent* DefenderASC = DefenderCharacter->GetAbilitySystemComponent();
	if (!DefenderASC)
	{
		return false;
	}

	if (!DeflectTag.IsValid() || !DefenderASC->HasMatchingGameplayTag(DeflectTag))
	{
		return false;
	}

	return IsWithinFacingAngle(DefenderCharacter, AttackerCharacter, MinDotThreshold);
}

void UNexusCombatFunctionLibrary::FilterHitResultsToLivingEnemyCharacters(
	const ANexusCharacterBase* SourceCharacter,
	const TArray<FHitResult>& InHitResults,
	TArray<ANexusCharacterBase*>& OutCharacters,
	TArray<FHitResult>& OutValidHitResults)
{
	OutCharacters.Reset();
	OutValidHitResults.Reset();

	if (!IsValid(SourceCharacter))
	{
		return;
	}

	for (const FHitResult& Hit : InHitResults)
	{
		ANexusCharacterBase* HitCharacter = GetNexusCharacterFromActor(Hit.GetActor());
		if (!IsValidLivingEnemyTarget(SourceCharacter, HitCharacter))
		{
			continue;
		}

		if (!OutCharacters.Contains(HitCharacter))
		{
			OutCharacters.Add(HitCharacter);
			OutValidHitResults.Add(Hit);
		}
	}
}

FGameplayCueParameters UNexusCombatFunctionLibrary::MakeImpactCueParameters(
	const FHitResult& HitResult,
	AActor* InstigatorActor,
	AActor* EffectCauserActor)
{
	FGameplayCueParameters Parameters;
	Parameters.Location = HitResult.ImpactPoint;
	Parameters.Normal = HitResult.ImpactNormal;
	Parameters.Instigator = InstigatorActor;
	Parameters.EffectCauser = EffectCauserActor ? EffectCauserActor : InstigatorActor;
	return Parameters;
}