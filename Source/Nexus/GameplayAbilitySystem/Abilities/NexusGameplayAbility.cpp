#include "NexusGameplayAbility.h"

#include "AbilitySystemComponent.h"
#include "Nexus/NexusEnumTypes.h"
#include "Nexus/Character/NexusCharacterBase.h"
#include "Nexus/DataAssets/AbilityInfo.h"
#include "Nexus/DataAssets/CostAndCooldownConfig.h"

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

	// Cost values are authored as positive logical values in the data asset.
	// Convert to negative additive GE magnitude here.
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