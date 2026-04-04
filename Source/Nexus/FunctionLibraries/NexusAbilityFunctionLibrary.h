#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "NexusAbilityFunctionLibrary.generated.h"

class AActor;
class ANexusCharacterBase;
class ANexusPlayerController;
class UNexusAbilitySystemComponent;
class UNexusGameplayAbility;

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