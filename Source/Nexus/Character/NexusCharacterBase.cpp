#include "NexusCharacterBase.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AIController.h"
#include "GameplayCueFunctionLibrary.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Net/UnrealNetwork.h"
#include "Nexus/NexusGameplayTags.h"
#include "Nexus/DataAssets/AbilityInfo.h"
#include "Nexus/DataAssets/GameplayEffectClassSet.h"
#include "Nexus/GameplayAbilitySystem/Abilities/NexusGameplayAbility.h"
#include "Nexus/GameplayAbilitySystem/AbilitySystemComponent/NexusAbilitySystemComponent.h"
#include "Nexus/GameplayAbilitySystem/AttributeSet/BasicAttributeSet.h"
#include "Nexus/PlayerState/NexusPlayerState.h"

ANexusCharacterBase::ANexusCharacterBase()
{
	PrimaryActorTick.bCanEverTick = false;

	AbilitySystemComponent = CreateDefaultSubobject<UNexusAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(AbilityReplicationMode);

	BasicAttributeSet = CreateDefaultSubobject<UBasicAttributeSet>(TEXT("BasicAttributeSet"));

	GetCapsuleComponent()->InitCapsuleSize(35.f, 90.f);

	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	MoveComp->JumpZVelocity = 500.f;
	MoveComp->AirControl = 0.35f;
	MoveComp->MaxWalkSpeed = 500.f;
	MoveComp->MinAnalogWalkSpeed = 20.f;
	MoveComp->BrakingDecelerationWalking = 2000.f;
	MoveComp->BrakingDecelerationFalling = 1500.f;
}

void ANexusCharacterBase::BeginPlay()
{
	Super::BeginPlay();
}

void ANexusCharacterBase::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	SetOwner(NewController);

	InitializeAbilityActorInfo();
	InitializeFromPlayerState();

	if (HasAuthority())
	{
		InitializeCombatLoadout();
	}
}

void ANexusCharacterBase::OnRep_Controller()
{
	Super::OnRep_Controller();

	if (Controller)
	{
		SetOwner(Controller);
	}

	InitializeAbilityActorInfo();
}


void ANexusCharacterBase::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	InitializeAbilityActorInfo();
	InitializeFromPlayerState();
}

void ANexusCharacterBase::InitializeAbilityActorInfo()
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
		AbilitySystemComponent->RefreshAbilityActorInfo();
	}
}

void ANexusCharacterBase::InitializeFromPlayerState()
{
	if (const ANexusPlayerState* PS = GetPlayerState<ANexusPlayerState>())
	{
		SetTeamID(PS->GetTeamID());
	}
}

void ANexusCharacterBase::InitializeCombatLoadout()
{
	if (BaseAbilities.Num() > 0)
	{
		GrantAbilitySet(ENexusAbilitySource::Base, BaseAbilities);
	}
}

UAbilitySystemComponent* ANexusCharacterBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ANexusCharacterBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, TeamID);
	DOREPLIFETIME(ThisClass, bIsDead);
	DOREPLIFETIME(ThisClass, CurrentCapturePoint);
}

TArray<ANexusCharacterBase*> ANexusCharacterBase::PerformSphereTraceForValidEnemies(
	const FVector& StartLocation,
	const FVector& EndLocation,
	float Radius,
	TArray<FHitResult>& OutValidHitResults)
{
	OutValidHitResults.Reset();

	if (!GetWorld())
	{
		return {};
	}

	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(this);

	TArray<FHitResult> RawHitResults;

	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));

	TArray<ANexusCharacterBase*> HitCharacters;

	const bool bHit = UKismetSystemLibrary::SphereTraceMultiForObjects(
		GetWorld(),
		StartLocation,
		EndLocation,
		Radius,
		ObjectTypes,
		false,
		ActorsToIgnore,
		EDrawDebugTrace::ForDuration,
		RawHitResults,
		true,
		FLinearColor::Red,
	FLinearColor::Green,
	1.f
	);

	if (!bHit)
	{
		return HitCharacters;
	}

	for (const FHitResult& Hit : RawHitResults)
	{
		ANexusCharacterBase* HitCharacter = Cast<ANexusCharacterBase>(Hit.GetActor());
		if (!IsValid(HitCharacter) || HitCharacter->GetIsDead())
		{
			continue;
		}

		if (HitCharacter->GetTeamID() == GetTeamID())
		{
			continue;
		}

		if (!HitCharacters.Contains(HitCharacter))
		{
			HitCharacters.Add(HitCharacter);
			OutValidHitResults.Add(Hit);
		}
	}

	return HitCharacters;
}

void ANexusCharacterBase::ApplyGameplayEffectSpecsToTarget(
	const TArray<FGameplayEffectSpecHandle>& EffectSpecHandles,
	ANexusCharacterBase* TargetToEffect)
{
	if (!HasAuthority() || !IsValid(TargetToEffect))
	{
		return;
	}

	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponent();
	UAbilitySystemComponent* TargetASC = TargetToEffect->GetAbilitySystemComponent();

	if (!TargetASC)
	{
		return;
	}

	for (const FGameplayEffectSpecHandle& EffectSpecHandle : EffectSpecHandles)
	{
		if (!EffectSpecHandle.IsValid() || !EffectSpecHandle.Data.IsValid())
		{
			continue;
		}

		if (SourceASC)
		{
			SourceASC->ApplyGameplayEffectSpecToTarget(*EffectSpecHandle.Data.Get(), TargetASC);
		}
		else
		{
			TargetASC->ApplyGameplayEffectSpecToSelf(*EffectSpecHandle.Data.Get());
		}
	}
}

void ANexusCharacterBase::ApplyGameplayEffectSpecsToTargets(
	const TArray<FGameplayEffectSpecHandle>& EffectSpecHandles,
	const TArray<ANexusCharacterBase*>& Targets)
{
	for (ANexusCharacterBase* Target : Targets)
	{
		ApplyGameplayEffectSpecsToTarget(EffectSpecHandles, Target);
	}
}

void ANexusCharacterBase::ApplySetByCallerEffectToTarget(
	ANexusCharacterBase* Target,
	TSubclassOf<UGameplayEffect> EffectClass,
	float Magnitude,
	FGameplayTag DataTag)
{
	if (!HasAuthority() || !IsValid(Target) || !EffectClass || !DataTag.IsValid())
	{
		return;
	}

	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponent();
	UAbilitySystemComponent* TargetASC = Target->GetAbilitySystemComponent();

	if (!SourceASC || !TargetASC)
	{
		return;
	}

	FGameplayEffectContextHandle ContextHandle = SourceASC->MakeEffectContext();
	ContextHandle.AddSourceObject(this);
	ContextHandle.AddInstigator(this, this);

	FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(EffectClass, 1.f, ContextHandle);
	if (!SpecHandle.IsValid() || !SpecHandle.Data.IsValid())
	{
		return;
	}

	SpecHandle.Data->SetSetByCallerMagnitude(DataTag, Magnitude);
	SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
}

void ANexusCharacterBase::ApplyStunToTarget(ANexusCharacterBase* Target, float Duration)
{
	if (!EffectSet || Duration <= 0.f)
	{
		return;
	}

	ApplySetByCallerEffectToTarget(
		Target,
		EffectSet->StunEffect,
		Duration,
		NexusGameplayTags::Data_Value_Duration
	);
}

void ANexusCharacterBase::ApplyDamageToTarget(ANexusCharacterBase* Target, float Damage)
{
	if (!EffectSet || !IsValid(Target))
	{
		return;
	}

	if (Damage <= 0.f)
	{
		return;
	}
	Damage = Damage *= -1;

	ApplySetByCallerEffectToTarget(
		Target,
		EffectSet->DamageEffect,
		Damage,
		NexusGameplayTags::Data_Value_Damage
	);
}

bool ANexusCharacterBase::CanParryMeleeHit(
	ANexusCharacterBase* Attacker) const
{
	if (!IsValid(Attacker) || GetIsDead())
	{
		return false;
	}

	if (!AbilitySystemComponent)
	{
		return false;
	}

	if (!AbilitySystemComponent->HasMatchingGameplayTag(NexusGameplayTags::Status_Defense_Deflecting))
	{
		return false;
	}

	if (!AbilitySystemComponent->HasMatchingGameplayTag(NexusGameplayTags::Status_Defense_DeflectWindow))
	{
		return false;
	}

	if (!IsAttackWithinDeflectAngle(Attacker, 0.2f))
	{
		return false;
	}

	return true;
}

bool ANexusCharacterBase::IsAttackWithinDeflectAngle(
	ANexusCharacterBase* Attacker,
	float MinDotThreshold) const
{
	if (!IsValid(Attacker))
	{
		return false;
	}

	const FVector DefenderForward = GetActorForwardVector().GetSafeNormal();
	const FVector ToAttacker = (Attacker->GetActorLocation() - GetActorLocation()).GetSafeNormal();

	const float Dot = FVector::DotProduct(DefenderForward, ToAttacker);
	return Dot >= MinDotThreshold;
}
bool ANexusCharacterBase::SendParryGameplayEvent(
	ANexusCharacterBase* Attacker,
	const FHitResult& HitResult)
{
	if (!HasAuthority() || !IsValid(Attacker) || !AbilitySystemComponent)
	{
		return false;
	}

	const FGameplayTag ParryEventTag = NexusGameplayTags::Event_Parry_Activated;

	FGameplayEventData Payload;
	Payload.EventTag = ParryEventTag;
	Payload.Instigator = Attacker;
	Payload.Target = this;
	Payload.TargetData = UAbilitySystemBlueprintLibrary::AbilityTargetDataFromHitResult(HitResult);
	Payload.OptionalObject = Attacker;

	const int32 NumTriggered = AbilitySystemComponent->HandleGameplayEvent(ParryEventTag, &Payload);

	UE_LOG(LogTemp, Warning,
		TEXT("HandleGameplayEvent Defender=%s TriggeredAbilities=%d"),
		*GetName(),
		NumTriggered);

	return NumTriggered > 0;
}

ENexusHitResolutionResult ANexusCharacterBase::ResolveMeleeHit(
	ANexusCharacterBase* Target,
	const FHitResult& HitResult,
	float Damage)
{
	if (!HasAuthority())
	{
		return ENexusHitResolutionResult::None;
	}

	if (!IsValid(Target) || Target->GetIsDead())
	{
		return ENexusHitResolutionResult::None;
	}

	if (Target->CanParryMeleeHit(this))
	{
		const bool bParryTriggered = Target->SendParryGameplayEvent(this, HitResult);
		if (bParryTriggered)
		{
			return ENexusHitResolutionResult::Parried;
		}
	}
	
	ApplyDamageToTarget(Target, Damage);
	
	FGameplayCueParameters Parameters;
	Parameters.Location = HitResult.ImpactPoint;
	Parameters.Normal = HitResult.ImpactNormal;
	Parameters.Instigator = this;

	UGameplayCueFunctionLibrary::ExecuteGameplayCueOnActor(
		Target,
		NexusGameplayTags::GameplayCue_Damage_Burst,
		Parameters);
	
	return ENexusHitResolutionResult::Damaged;
}

void ANexusCharacterBase::DebugLogGrantedAbilities(const FString& Context) const
{
	if (!AbilitySystemComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] %s has no ASC"), *Context, *GetName());
		return;
	}

	TArray<FGameplayAbilitySpecHandle> Handles;
	AbilitySystemComponent->GetAllAbilities(Handles);

	UE_LOG(LogTemp, Warning, TEXT("[%s] Granted abilities for %s"), *Context, *GetName());

	for (const FGameplayAbilitySpecHandle& Handle : Handles)
	{
		const FGameplayAbilitySpec* Spec = AbilitySystemComponent->FindAbilitySpecFromHandle(Handle);
		if (!Spec || !Spec->Ability)
		{
			continue;
		}

		UE_LOG(LogTemp, Warning, TEXT("  %s"), *Spec->Ability->GetClass()->GetName());
	}
}

void ANexusCharacterBase::SetTeamID(ENexusTeamID NewTeamID)
{
	if (TeamID == NewTeamID)
	{
		return;
	}

	TeamID = NewTeamID;

	if (HasAuthority())
	{
		HandleTeamChanged();
	}
}

void ANexusCharacterBase::OnRep_TeamID()
{
	HandleTeamChanged();
}

void ANexusCharacterBase::HandleTeamChanged()
{
	ApplyTeamVisuals();
	OnCombatStateChanged.Broadcast();
}

void ANexusCharacterBase::ApplyTeamVisuals() const
{
	// Intentionally empty in base.
}

bool ANexusCharacterBase::IsFriendlyTo(AActor* OtherActor) const
{
	const ANexusCharacterBase* OtherCharacter = Cast<ANexusCharacterBase>(OtherActor);
	return OtherCharacter && TeamID == OtherCharacter->TeamID;
}

bool ANexusCharacterBase::IsEnemyTo(AActor* OtherActor) const
{
	const ANexusCharacterBase* OtherCharacter = Cast<ANexusCharacterBase>(OtherActor);
	return OtherCharacter && TeamID != OtherCharacter->TeamID;
}

void ANexusCharacterBase::SetCurrentCapturePoint(ANexusCapturePoint* NewCapturePoint)
{
	if (CurrentCapturePoint == NewCapturePoint)
	{
		return;
	}

	CurrentCapturePoint = NewCapturePoint;

	if (HasAuthority())
	{
		HandleCurrentCapturePointChanged();
	}
}

void ANexusCharacterBase::OnRep_CurrentCapturePoint()
{
	HandleCurrentCapturePointChanged();
}

void ANexusCharacterBase::HandleCurrentCapturePointChanged()
{
	OnCombatStateChanged.Broadcast();
}

void ANexusCharacterBase::Die()
{
	if (!HasAuthority() || bIsDead)
	{
		return;
	}

	bIsDead = true;

	ApplyDeathState_Server();

	// Fire the animation trigger once from the server
	Multicast_DeathAnimation();

	HandleDeathStateChanged();
	ForceNetUpdate();
}

void ANexusCharacterBase::Multicast_DeathAnimation_Implementation()
{
	PlayDeathAnimation();
}

void ANexusCharacterBase::PlayDeathAnimation()
{
	if (!DeathMontage)
	{
		return;
	}

	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		if (UAnimInstance* AnimInstance = MeshComp->GetAnimInstance())
		{
			AnimInstance->StopAllMontages(0.1f);
			AnimInstance->Montage_Play(DeathMontage);
		}
	}
}

void ANexusCharacterBase::ApplyDeathState_Server()
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->CancelAllAbilities();
	}

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->DisableMovement();
		MoveComp->StopMovementImmediately();
	}

	GetWorldTimerManager().ClearAllTimersForObject(this);

	if (AAIController* AI = Cast<AAIController>(GetController()))
	{
		AI->StopMovement();
	}
}

void ANexusCharacterBase::OnRep_IsDead()
{
	HandleDeathStateChanged();
}

void ANexusCharacterBase::HandleDeathStateChanged()
{
	RefreshDeadGameplayTag();
	ApplyDeathPresentation();
	OnCombatStateChanged.Broadcast();
}

void ANexusCharacterBase::RefreshDeadGameplayTag()
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	const bool bHasDeadTag = AbilitySystemComponent->HasMatchingGameplayTag(NexusGameplayTags::Status_Dead);

	if (bIsDead && !bHasDeadTag)
	{
		AbilitySystemComponent->AddLooseGameplayTag(NexusGameplayTags::Status_Dead);
	}
	else if (!bIsDead && bHasDeadTag)
	{
		AbilitySystemComponent->RemoveLooseGameplayTag(NexusGameplayTags::Status_Dead);
	}
}

void ANexusCharacterBase::ApplyDeathPresentation()
{
	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->StopMovementImmediately();

	GetCapsuleComponent()->SetCollisionObjectType(ECC_WorldStatic);
	GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);

	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->SetCollisionObjectType(ECC_WorldStatic);
		MeshComp->SetCollisionResponseToAllChannels(ECR_Ignore);
		MeshComp->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	}

	if (IsLocallyControlled())
	{
		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			DisableInput(PC);
		}
	}

	BP_OnDeathStarted();
}

void ANexusCharacterBase::EnterRagdoll()
{
	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->StopMovementImmediately();

	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->SetCollisionProfileName(TEXT("Ragdoll"));
		MeshComp->SetAllBodiesSimulatePhysics(true);
		MeshComp->SetSimulatePhysics(true);
		MeshComp->bBlendPhysics = true;
	}
	SetLifeSpan(4.f);
}

TArray<FGameplayAbilitySpecHandle>& ANexusCharacterBase::GetHandleArrayForSource(ENexusAbilitySource Source)
{
	switch (Source)
	{
	case ENexusAbilitySource::Base:
		return BaseAbilityHandles;
	case ENexusAbilitySource::Class:
		return ClassAbilityHandles;
	case ENexusAbilitySource::Weapon:
		return WeaponAbilityHandles;
	default:
		return BaseAbilityHandles;
	}
}

bool ANexusCharacterBase::HasGrantedAbilityClass(TSubclassOf<UNexusGameplayAbility> AbilityClass) const
{
	if (!AbilitySystemComponent || !AbilityClass)
	{
		return false;
	}

	TArray<FGameplayAbilitySpecHandle> ExistingHandles;
	AbilitySystemComponent->GetAllAbilities(ExistingHandles);

	for (const FGameplayAbilitySpecHandle& Handle : ExistingHandles)
	{
		const FGameplayAbilitySpec* Spec = AbilitySystemComponent->FindAbilitySpecFromHandle(Handle);
		if (Spec && Spec->Ability && Spec->Ability->GetClass() == AbilityClass)
		{
			return true;
		}
	}

	return false;
}

bool ANexusCharacterBase::HasGrantedAbilityTag(const FGameplayTag& AbilityTag) const
{
	if (!AbilitySystemComponent || !AbilityTag.IsValid())
	{
		return false;
	}

	TArray<FGameplayAbilitySpecHandle> ExistingHandles;
	AbilitySystemComponent->GetAllAbilities(ExistingHandles);

	for (const FGameplayAbilitySpecHandle& Handle : ExistingHandles)
	{
		const FGameplayAbilitySpec* Spec = AbilitySystemComponent->FindAbilitySpecFromHandle(Handle);
		if (Spec && Spec->GetDynamicSpecSourceTags().HasTagExact(AbilityTag))
		{
			return true;
		}
	}

	return false;
}

TArray<FGameplayAbilitySpecHandle> ANexusCharacterBase::GrantAbilitySet(
	ENexusAbilitySource Source,
	const TArray<TSubclassOf<UNexusGameplayAbility>>& AbilitiesToGrant)
{
	TArray<FGameplayAbilitySpecHandle> NewHandles;

	if (!HasAuthority() || !AbilitySystemComponent)
	{
		return NewHandles;
	}

	TArray<FGameplayAbilitySpecHandle>& HandleArray = GetHandleArrayForSource(Source);

	for (const TSubclassOf<UNexusGameplayAbility>& AbilityClass : AbilitiesToGrant)
	{
		if (!AbilityClass || HasGrantedAbilityClass(AbilityClass))
		{
			continue;
		}

		const UNexusGameplayAbility* AbilityCDO = AbilityClass->GetDefaultObject<UNexusGameplayAbility>();
		if (!AbilityCDO)
		{
			continue;
		}

		FGameplayAbilitySpec Spec(AbilityClass, 1, INDEX_NONE, this);

		if (AbilityCDO->AbilityTagConfig.InputTag.IsValid())
		{
			Spec.GetDynamicSpecSourceTags().AddTag(AbilityCDO->AbilityTagConfig.InputTag);
		}

		if (AbilityCDO->AbilityTagConfig.AbilityTag.IsValid())
		{
			Spec.GetDynamicSpecSourceTags().AddTag(AbilityCDO->AbilityTagConfig.AbilityTag);
		}

		if (AbilityCDO->AbilityTagConfig.WeaponTag.IsValid())
		{
			Spec.GetDynamicSpecSourceTags().AddTag(AbilityCDO->AbilityTagConfig.WeaponTag);
		}

		const FGameplayAbilitySpecHandle Handle = AbilitySystemComponent->GiveAbility(Spec);

		if (Handle.IsValid())
		{
			HandleArray.Add(Handle);
			NewHandles.Add(Handle);
		}
	}

	if (NewHandles.Num() > 0)
	{
		AbilitySystemComponent->ForceReplication();
		ForceNetUpdate();
		OnCombatStateChanged.Broadcast();
	}

	return NewHandles;
}

void ANexusCharacterBase::ClearAbilitySet(ENexusAbilitySource Source)
{
	if (!HasAuthority() || !AbilitySystemComponent)
	{
		return;
	}

	TArray<FGameplayAbilitySpecHandle>& HandleArray = GetHandleArrayForSource(Source);

	bool bChanged = false;

	for (const FGameplayAbilitySpecHandle& Handle : HandleArray)
	{
		if (Handle.IsValid())
		{
			AbilitySystemComponent->ClearAbility(Handle);
			bChanged = true;
		}
	}

	HandleArray.Reset();

	if (bChanged)
	{
		AbilitySystemComponent->ForceReplication();
		ForceNetUpdate();
		OnCombatStateChanged.Broadcast();
	}
}

void ANexusCharacterBase::ClearAllGrantedAbilitySets()
{
	ClearAbilitySet(ENexusAbilitySource::Base);
	ClearAbilitySet(ENexusAbilitySource::Class);
	ClearAbilitySet(ENexusAbilitySource::Weapon);
}