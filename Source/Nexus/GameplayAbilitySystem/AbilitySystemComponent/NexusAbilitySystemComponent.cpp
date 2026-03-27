// Fill out your copyright notice in the Description page of Project Settings.

#include "NexusAbilitySystemComponent.h"

#include "Nexus/Character/NexusCharacterBase.h"

UNexusAbilitySystemComponent::UNexusAbilitySystemComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UNexusAbilitySystemComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UNexusAbilitySystemComponent::OnRep_ActivateAbilities()
{
	Super::OnRep_ActivateAbilities();

	if (ANexusCharacterBase* Character = Cast<ANexusCharacterBase>(GetOwner()))
	{
		if (LastActivatedAbilitySpecs.Num() != ActivatableAbilities.Items.Num())
		{
			Character->BroadcastAbilitiesChanged();
			LastActivatedAbilitySpecs = ActivatableAbilities.Items;
		}
		else
		{
			for (int32 i =0; i < LastActivatedAbilitySpecs.Num(); i++)
			{
				if (LastActivatedAbilitySpecs[i].Ability != ActivatableAbilities.Items[i].Ability)
				{
					Character->BroadcastAbilitiesChanged();
					LastActivatedAbilitySpecs = ActivatableAbilities.Items;
					break;
				}
			}
		}
	}
}

void UNexusAbilitySystemComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                                 FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

