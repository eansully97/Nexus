#include "NexusGameplayAbility.h"

#include "AbilitySystemComponent.h"
#include "GameFramework/Pawn.h"
#include "Nexus/NexusEnumTypes.h"
#include "Nexus/Character/NexusCharacterBase.h"
#include "Nexus/Character/Player/NexusPlayerCharacter.h"
#include "Nexus/Controller/NexusPlayerController.h"
#include "Nexus/DataAssets/AbilityInfo.h"
#include "Nexus/DataAssets/CostAndCooldownConfig.h"
#include "Nexus/FunctionLibraries/NexusAbilityFunctionLibrary.h"
#include "Nexus/GameplayAbilitySystem/AbilitySystemComponent/NexusAbilitySystemComponent.h"

UNexusGameplayAbility::UNexusGameplayAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

bool UNexusGameplayAbility::CommitAbilityWithSetByCaller(
	float CooldownDuration,
	float CostAmount)
{
	if (!CommitCheck(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo))
	{
		return false;
	}

	ApplyCostSetByCaller(CostAmount);
	ApplyCooldownSetByCaller(CooldownDuration);

	CommitExecute(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo);
	return true;
}

FText UNexusGameplayAbility::GetAbilityDisplayName() const
{
	return AbilityInfo ? AbilityInfo->AbilityDisplayName : FText();
}

EAbilityContainerInfo UNexusGameplayAbility::GetContainerToShowIn() const
{
	return AbilityInfo ? AbilityInfo->ContainerToShowIn : EAbilityContainerInfo::None;
}

ANexusCharacterBase* UNexusGameplayAbility::GetNexusCharacterFromActorInfo() const
{
	return Cast<ANexusCharacterBase>(GetAvatarActorFromActorInfo());
}

ANexusPlayerCharacter* UNexusGameplayAbility::GetNexusPlayerCharacterFromActorInfo() const
{
	return Cast<ANexusPlayerCharacter>(GetAvatarActorFromActorInfo());
}

ANexusPlayerController* UNexusGameplayAbility::GetNexusPlayerControllerFromActorInfo() const
{
	if (const APawn* Pawn = Cast<APawn>(GetAvatarActorFromActorInfo()))
	{
		return Cast<ANexusPlayerController>(Pawn->GetController());
	}

	return Cast<ANexusPlayerController>(GetOwningActorFromActorInfo());
}

UNexusAbilitySystemComponent* UNexusGameplayAbility::GetNexusAbilitySystemComponentFromActorInfo() const
{
	return Cast<UNexusAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
}

FGameplayTag UNexusGameplayAbility::GetPrimaryActivationEventTag() const
{
	for (const FAbilityTriggerData& TriggerData : AbilityTriggers)
	{
		if (TriggerData.TriggerSource == EGameplayAbilityTriggerSource::GameplayEvent &&
			TriggerData.TriggerTag.IsValid())
		{
			return TriggerData.TriggerTag;
		}
	}

	return FGameplayTag();
}

UAbilitySystemComponent* UNexusGameplayAbility::GetSourceAbilitySystemComponent() const
{
	return GetAbilitySystemComponentFromActorInfo();
}

ANexusCharacterBase* UNexusGameplayAbility::GetTargetCharacterFromEventData(
	const FGameplayEventData* TriggerEventData) const
{
	if (!TriggerEventData)
	{
		return nullptr;
	}

	return Cast<ANexusCharacterBase>(const_cast<AActor*>(TriggerEventData->Target.Get()));
}

bool UNexusGameplayAbility::IsAbilityTargetUsable(
	const ANexusCharacterBase* SourceCharacter,
	const ANexusCharacterBase* TargetCharacter) const
{
	return UNexusAbilityFunctionLibrary::IsAbilityTargetUsable(this, SourceCharacter, TargetCharacter);
}

bool UNexusGameplayAbility::IsAbilityTargetUsable(const ANexusCharacterBase* TargetCharacter) const
{
	return IsAbilityTargetUsable(GetNexusCharacterFromActorInfo(), TargetCharacter);
}

bool UNexusGameplayAbility::TryGetUsableControllerTarget(ANexusCharacterBase*& OutTargetCharacter) const
{
	return UNexusAbilityFunctionLibrary::TryGetUsableControllerTargetForAbility(
		GetNexusPlayerControllerFromActorInfo(),
		GetNexusCharacterFromActorInfo(),
		this,
		OutTargetCharacter);
}

bool UNexusGameplayAbility::AbilityRequiresUsableTarget() const
{
	return bRequiresValidTarget;
}

float UNexusGameplayAbility::GetEffectiveActivationRange() const
{
	return FMath::Max(0.f, ActivationRange);
}

bool UNexusGameplayAbility::ApplySetByCallerEffectToOwner(
	TSubclassOf<UGameplayEffect> EffectClass,
	const FGameplayTag& DataTag,
	float Magnitude) const
{
	UAbilitySystemComponent* ASC = GetSourceAbilitySystemComponent();

	if (!EffectClass ||
		!DataTag.IsValid() ||
		!FMath::IsFinite(Magnitude) ||
		!ASC)
	{
		return false;
	}

	FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(EffectClass, GetAbilityLevel());
	if (!SpecHandle.IsValid() || !SpecHandle.Data.IsValid())
	{
		return false;
	}

	SpecHandle.Data->SetSetByCallerMagnitude(DataTag, Magnitude);
	ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	return true;
}

void UNexusGameplayAbility::ApplyCooldownSetByCaller(float InDuration)
{
	if (!CostAndCooldownConfig || !CostAndCooldownConfig->CooldownEffectClass)
	{
		return;
	}

	const float FinalDuration = InDuration >= 0.f
		? InDuration
		: CostAndCooldownConfig->CooldownDuration;

	if (FinalDuration <= 0.f)
	{
		return;
	}

	ApplySetByCallerEffectToOwner(
		CostAndCooldownConfig->CooldownEffectClass,
		CostAndCooldownConfig->CooldownSetByCallerTag,
		FinalDuration);
}

void UNexusGameplayAbility::ApplyCostSetByCaller(float InCostAmount)
{
	if (!CostAndCooldownConfig || !CostAndCooldownConfig->CostEffectClass)
	{
		return;
	}

	const float LogicalCost = InCostAmount >= 0.f
		? InCostAmount
		: CostAndCooldownConfig->CostAmount;

	if (LogicalCost <= 0.f)
	{
		return;
	}

	ApplySetByCallerEffectToOwner(
		CostAndCooldownConfig->CostEffectClass,
		CostAndCooldownConfig->CostSetByCallerTag,
		-LogicalCost);
}

void UNexusGameplayAbility::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}