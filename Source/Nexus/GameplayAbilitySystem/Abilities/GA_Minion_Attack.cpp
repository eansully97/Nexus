#include "GA_Minion_Attack.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Animation/AnimMontage.h"
#include "Nexus/NexusGameplayTags.h"
#include "Nexus/Character/Enemy/Minions/NexusMinionBase.h"
#include "UObject/ConstructorHelpers.h"

UGA_Minion_Attack::UGA_Minion_Attack()
{
	ActivationOwnedTags.AddTag(NexusGameplayTags::Status_Ability_Active);
	HitScanStartEventTag = NexusGameplayTags::Event_HitScan_Start;
	HitScanEndEventTag = NexusGameplayTags::Event_HitScan_End;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(NexusGameplayTags::Ability_Enemy_Attack);
	SetAssetTags(AssetTags);

	static ConstructorHelpers::FObjectFinder<UAnimMontage> MinionAttackMontage(
		TEXT("/Game/ParagonMinions/Characters/Minions/Down_Minions/Animations/Melee/AM_Minion_Attack1.AM_Minion_Attack1"));
	if (MinionAttackMontage.Succeeded())
	{
		MontageToPlay = MinionAttackMontage.Object;
	}
}

void UGA_Minion_Attack::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	CachedMinion = nullptr;
	bHitscanWindowOpen = false;
	CleanupTasks();

	if (!ActorInfo || !ActorInfo->AvatarActor.IsValid() || !ActorInfo->IsNetAuthority() || !MontageToPlay)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ANexusMinionBase* ResolvedMinion = nullptr;
	if (!TryResolveMinion(ResolvedMinion))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	CachedMinion = ResolvedMinion;

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

void UGA_Minion_Attack::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	EndHitscanWindow();
	CleanupTasks();
	CachedMinion = nullptr;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_Minion_Attack::CleanupTasks()
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

void UGA_Minion_Attack::EndHitscanWindow()
{
	if (!bHitscanWindowOpen || !IsValid(CachedMinion))
	{
		bHitscanWindowOpen = false;
		return;
	}

	CachedMinion->EndHitscan();
	bHitscanWindowOpen = false;
}

bool UGA_Minion_Attack::TryResolveMinion(ANexusMinionBase*& OutMinion) const
{
	OutMinion = Cast<ANexusMinionBase>(GetAvatarActorFromActorInfo());
	return IsValid(OutMinion);
}

void UGA_Minion_Attack::OnHitScanStart(FGameplayEventData Payload)
{
	if (!IsValid(CachedMinion))
	{
		ANexusMinionBase* ResolvedMinion = nullptr;
		if (TryResolveMinion(ResolvedMinion))
		{
			CachedMinion = ResolvedMinion;
		}
	}

	if (!IsValid(CachedMinion))
	{
		return;
	}

	CachedMinion->StartHitscan();
	bHitscanWindowOpen = true;
}

void UGA_Minion_Attack::OnHitScanEnd(FGameplayEventData Payload)
{
	EndHitscanWindow();
}

void UGA_Minion_Attack::OnMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_Minion_Attack::OnMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UGA_Minion_Attack::OnMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}
