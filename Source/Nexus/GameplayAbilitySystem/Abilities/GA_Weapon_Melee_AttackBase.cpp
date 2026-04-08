#include "GA_Weapon_Melee_AttackBase.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Animation/AnimMontage.h"
#include "Nexus/NexusGameplayTags.h"
#include "Nexus/Character/Player/NexusPlayerCharacter.h"
#include "Nexus/Components/NexusWeaponsManager.h"
#include "Nexus/Weapons/Melee/NexusMeleeWeaponBase.h"

UGA_Weapon_Melee_AttackBase::UGA_Weapon_Melee_AttackBase()
{
	ActivationOwnedTags.AddTag(NexusGameplayTags::Status_Ability_Active);
	HitScanStartEventTag = NexusGameplayTags::Event_HitScan_Start;
	HitScanEndEventTag = NexusGameplayTags::Event_HitScan_End;
}

void UGA_Weapon_Melee_AttackBase::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	CachedMeleeWeapon = nullptr;
	bHitscanWindowOpen = false;
	CleanupTasks();

	if (!ActorInfo || !ActorInfo->AvatarActor.IsValid() || !MontageToPlay)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ANexusMeleeWeaponBase* ResolvedMeleeWeapon = nullptr;
	if (!TryGetEquippedMeleeWeapon(ResolvedMeleeWeapon))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	CachedMeleeWeapon = ResolvedMeleeWeapon;

	ConfigureWeaponForAttack(CachedMeleeWeapon);

	if (!CommitAbilityWithSetByCaller())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (HitScanStartEventTag.IsValid())
	{
		StartHitScanTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this,
			HitScanStartEventTag,
			nullptr,
			false,
			true);

		if (StartHitScanTask)
		{
			StartHitScanTask->EventReceived.AddDynamic(this, &ThisClass::OnHitScanStart);
			StartHitScanTask->ReadyForActivation();
		}
	}

	if (HitScanEndEventTag.IsValid())
	{
		EndHitScanTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this,
			HitScanEndEventTag,
			nullptr,
			false,
			true);

		if (EndHitScanTask)
		{
			EndHitScanTask->EventReceived.AddDynamic(this, &ThisClass::OnHitScanEnd);
			EndHitScanTask->ReadyForActivation();
		}
	}

	MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		NAME_None,
		MontageToPlay,
		MontagePlayRate,
		MontageStartSection,
		bStopMontageWhenAbilityEnds,
		AnimRootMotionTranslationScale,
		MontageStartTimeSeconds,
		bAllowInterruptAfterBlendOut);

	if (!MontageTask)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageCompleted);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageCancelled);
	MontageTask->ReadyForActivation();
}

void UGA_Weapon_Melee_AttackBase::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	EndHitscanWindow();
	CleanupTasks();
	CachedMeleeWeapon = nullptr;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

bool UGA_Weapon_Melee_AttackBase::TryGetEquippedMeleeWeapon(ANexusMeleeWeaponBase*& OutMeleeWeapon) const
{
	OutMeleeWeapon = nullptr;

	const ANexusPlayerCharacter* PlayerCharacter = GetNexusPlayerCharacterFromActorInfo();
	const UNexusWeaponsManager* WeaponsManager = PlayerCharacter ? PlayerCharacter->GetWeaponsManager() : nullptr;
	OutMeleeWeapon = WeaponsManager ? Cast<ANexusMeleeWeaponBase>(WeaponsManager->EquippedWeapon) : nullptr;
	return IsValid(OutMeleeWeapon);
}

void UGA_Weapon_Melee_AttackBase::ConfigureWeaponForAttack(ANexusMeleeWeaponBase* InWeapon) const
{
	if (!IsValid(InWeapon))
	{
		return;
	}

	InWeapon->ConfigureHitscanWindow(DamageHitScanRadius, DamageToDeal);
}

void UGA_Weapon_Melee_AttackBase::EndHitscanWindow()
{
	if (!bHitscanWindowOpen || !IsValid(CachedMeleeWeapon))
	{
		bHitscanWindowOpen = false;
		return;
	}

	CachedMeleeWeapon->EndHitscan();
	bHitscanWindowOpen = false;
}

void UGA_Weapon_Melee_AttackBase::CleanupTasks()
{
	if (StartHitScanTask)
	{
		StartHitScanTask->EndTask();
		StartHitScanTask = nullptr;
	}

	if (EndHitScanTask)
	{
		EndHitScanTask->EndTask();
		EndHitScanTask = nullptr;
	}

	if (MontageTask)
	{
		MontageTask->EndTask();
		MontageTask = nullptr;
	}
}

void UGA_Weapon_Melee_AttackBase::OnHitScanStart(FGameplayEventData Payload)
{
	if (!IsValid(CachedMeleeWeapon))
	{
		ANexusMeleeWeaponBase* ResolvedMeleeWeapon = nullptr;
		if (TryGetEquippedMeleeWeapon(ResolvedMeleeWeapon))
		{
			CachedMeleeWeapon = ResolvedMeleeWeapon;
		}
	}

	if (!IsValid(CachedMeleeWeapon))
	{
		return;
	}

	ConfigureWeaponForAttack(CachedMeleeWeapon);
	CachedMeleeWeapon->StartHitscan();
	bHitscanWindowOpen = true;
}

void UGA_Weapon_Melee_AttackBase::OnHitScanEnd(FGameplayEventData Payload)
{
	EndHitscanWindow();
}

void UGA_Weapon_Melee_AttackBase::OnMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_Weapon_Melee_AttackBase::OnMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UGA_Weapon_Melee_AttackBase::OnMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}
