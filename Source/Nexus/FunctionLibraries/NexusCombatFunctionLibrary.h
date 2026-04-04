#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "NexusCombatFunctionLibrary.generated.h"

class AActor;
class ANexusCharacterBase;
class UAbilitySystemComponent;
struct FHitResult;
struct FGameplayTag;

UCLASS()
class NEXUS_API UNexusCombatFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static ANexusCharacterBase* GetNexusCharacterFromActor(const AActor* Actor);

	static bool IsCharacterAlive(const ANexusCharacterBase* Character);

	static bool IsValidLivingEnemyTarget(
		const ANexusCharacterBase* SourceCharacter,
		const ANexusCharacterBase* TargetCharacter);

	static bool IsWithinRange(
		const AActor* SourceActor,
		const AActor* TargetActor,
		float Range);

	static bool IsWithinFacingAngle(
		const AActor* DefenderActor,
		const AActor* AttackerActor,
		float MinDotThreshold);

	static bool CanCharacterDeflectMeleeHit(
		const ANexusCharacterBase* DefenderCharacter,
		const ANexusCharacterBase* AttackerCharacter,
		float MinDotThreshold,
		const FGameplayTag& DeflectTag);

	static void FilterHitResultsToLivingEnemyCharacters(
		const ANexusCharacterBase* SourceCharacter,
		const TArray<FHitResult>& InHitResults,
		TArray<ANexusCharacterBase*>& OutCharacters,
		TArray<FHitResult>& OutValidHitResults);

	static FGameplayCueParameters MakeImpactCueParameters(
		const FHitResult& HitResult,
		AActor* InstigatorActor,
		AActor* EffectCauserActor = nullptr);
};