#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "GameplayTagContainer.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "NexusAbilityFunctionLibrary.generated.h"

class AActor;
class ANexusCharacterBase;
class ANexusPlayerController;
class UNexusAbilitySystemComponent;
class UNexusGameplayAbility;

USTRUCT(BlueprintType)
struct FNexusAbilityTargetHit
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Nexus|Ability|Targeting")
	TObjectPtr<AActor> TargetActor = nullptr;

	UPROPERTY(BlueprintReadOnly, Category="Nexus|Ability|Targeting")
	FHitResult HitResult;
};

UCLASS()
class NEXUS_API UNexusAbilityFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static UNexusAbilitySystemComponent* GetNexusASCFromActor(const AActor* Actor);

	static ANexusPlayerController* GetNexusPlayerControllerFromActor(const AActor* Actor);

	static bool AbilityRequiresUsableTarget(const UNexusGameplayAbility* Ability);

	static bool IsAbilityTargetUsable(
		const UNexusGameplayAbility* Ability,
		const ANexusCharacterBase* SourceCharacter,
		const ANexusCharacterBase* TargetCharacter);

	static bool TryGetUsableControllerTargetForAbility(
		const ANexusPlayerController* Controller,
		const ANexusCharacterBase* SourceCharacter,
		const UNexusGameplayAbility* Ability,
		ANexusCharacterBase*& OutTargetCharacter);

	static bool SendTargetedGameplayEventToActor(
		AActor* SourceActor,
		const FGameplayTag& EventTag,
		AActor* TargetActor,
		AActor* OptionalObject = nullptr);

	static bool BuildTargetHitResultFromOrigin(
		const AActor* TargetActor,
		const FVector& Origin,
		FHitResult& OutHitResult);

	static bool HasLineOfSightToTarget(
		const UObject* WorldContextObject,
		const FVector& Origin,
		const AActor* TargetActor,
		ECollisionChannel TraceChannel = ECC_Visibility,
		const AActor* ActorToIgnore = nullptr,
		const AActor* AdditionalIgnoredActor = nullptr);

	static bool QueryRadialEnemyCharacterTargets(
		const UObject* WorldContextObject,
		const FVector& Origin,
		float Radius,
		const ANexusCharacterBase* SourceCharacter,
		TArray<FNexusAbilityTargetHit>& OutTargetHits,
		bool bRequireLineOfSight = false,
		ECollisionChannel LineOfSightTraceChannel = ECC_Visibility,
		const AActor* AdditionalIgnoredActor = nullptr);

	UFUNCTION(BlueprintCallable, Category="Nexus|Ability|GameplayCue")
	static bool AddGameplayCueToActor(
		AActor* TargetActor,
		const FGameplayTag& CueTag,
		AActor* InstigatorActor = nullptr,
		AActor* EffectCauserActor = nullptr,
		UObject* OptionalSourceObject = nullptr);

	UFUNCTION(BlueprintCallable, Category="Nexus|Ability|GameplayCue")
	static bool RemoveGameplayCueFromActor(
		AActor* TargetActor,
		const FGameplayTag& CueTag);

	UFUNCTION(BlueprintCallable, Category="Nexus|Ability|GameplayCue")
	static bool ExecuteGameplayCueOnActor(
		AActor* TargetActor,
		const FGameplayTag& CueTag,
		AActor* InstigatorActor = nullptr,
		AActor* EffectCauserActor = nullptr,
		UObject* OptionalSourceObject = nullptr);
};