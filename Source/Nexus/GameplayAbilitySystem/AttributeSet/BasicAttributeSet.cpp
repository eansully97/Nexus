// Fill out your copyright notice in the Description page of Project Settings.


#include "BasicAttributeSet.h"

#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"
#include "Nexus/Character/NexusCharacterBase.h"

UBasicAttributeSet::UBasicAttributeSet()
{
	Health = 100.0f;
	MaxHealth = 100.0f;
	MoveSpeed = 500.0f;
	JumpVelocity = 500.0f;
	AirControl = 0.35f;
}

void UBasicAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION_NOTIFY(UBasicAttributeSet, Health, COND_None, REPNOTIFY_Always)
	DOREPLIFETIME_CONDITION_NOTIFY(UBasicAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always)
	DOREPLIFETIME_CONDITION_NOTIFY(UBasicAttributeSet, MoveSpeed, COND_None, REPNOTIFY_Always)
	DOREPLIFETIME_CONDITION_NOTIFY(UBasicAttributeSet, JumpVelocity, COND_None, REPNOTIFY_Always)
	DOREPLIFETIME_CONDITION_NOTIFY(UBasicAttributeSet, AirControl, COND_None, REPNOTIFY_Always)
}

void UBasicAttributeSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);

	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHealth());
	}
	else if (Attribute == GetMaxHealthAttribute())
	{
		NewValue = FMath::Max(0.f, NewValue);
	}
	else if (Attribute == GetMoveSpeedAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, 2000.f);
	}
	else if (Attribute == GetJumpVelocityAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, 2000.f);
	}
	else if (Attribute == GetAirControlAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, 1.f);
	}
}

void UBasicAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		SetHealth(FMath::Clamp(GetHealth(), 0.f, GetMaxHealth()));

		if (GetHealth() <= 0.f)
		{
			ANexusCharacterBase* Character = Cast<ANexusCharacterBase>(GetOwningActor());
			if (Character)
			{
				Character->Die();
			}
		}
	}
	else if (Data.EvaluatedData.Attribute == GetMaxHealthAttribute())
	{
		SetMaxHealth(FMath::Max(0.f, GetMaxHealth()));
		SetHealth(FMath::Clamp(GetHealth(), 0.f, GetMaxHealth()));
	}
	else if (Data.EvaluatedData.Attribute == GetMoveSpeedAttribute())
	{
		SetMoveSpeed(FMath::Clamp(GetMoveSpeed(), 0.f, 2000.f));
	}
	else if (Data.EvaluatedData.Attribute == GetJumpVelocityAttribute())
	{
		SetJumpVelocity(FMath::Clamp(GetJumpVelocity(), 0.f, 2000.f));
	}
	else if (Data.EvaluatedData.Attribute == GetAirControlAttribute())
	{
		SetAirControl(FMath::Clamp(GetAirControl(), 0.f, 1.f));
	}
}
