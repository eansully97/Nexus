#include "NexusAbilityFunctionLibrary.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
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