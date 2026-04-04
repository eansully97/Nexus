#include "NexusCharacterBase.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AIController.h"
#include "Animation/AnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameplayCueFunctionLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Net/UnrealNetwork.h"
#include "Nexus/NexusGameplayTags.h"
#include "Nexus/DataAssets/GameplayEffectClassSet.h"
#include "Nexus/FunctionLibraries/NexusCombatFunctionLibrary.h"
#include "Nexus/GameplayAbilitySystem/Abilities/NexusGameplayAbility.h"
#include "Nexus/GameplayAbilitySystem/AbilitySystemComponent/NexusAbilitySystemComponent.h"
#include "Nexus/GameplayAbilitySystem/AttributeSet/BasicAttributeSet.h"
#include "Nexus/PlayerState/NexusPlayerState.h"

ANexusCharacterBase::ANexusCharacterBase()
{
	PrimaryActorTick.bCanEverTick = false;

	AbilitySystemComponent = CreateDefaultSubobject<UNexusAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);

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

void ANexusCharacterBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->SetReplicationMode(AbilityReplicationMode);
	}
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
	RebuildCombatLoadout();
}

TArray<FNexusAbilityGrant> ANexusCharacterBase::GetClassAbilitiesToGrant() const
{
	return {};
}

void ANexusCharacterBase::RebuildCombatLoadout()
{
	if (!HasAuthority())
	{
		return;
	}

	ClearAbilitySet(ENexusAbilitySource::Class);

	const TArray<FNexusAbilityGrant> ClassAbilityGrants = GetClassAbilitiesToGrant();
	if (ClassAbilityGrants.Num() > 0)
	{
		GrantAbilitySet(ENexusAbilitySource::Class, ClassAbilityGrants);
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

	const bool bHit = UKismetSystemLibrary::SphereTraceMultiForObjects(
		GetWorld(),
		StartLocation,
		EndLocation,
		Radius,
		ObjectTypes,
		false,
		ActorsToIgnore,
		EDrawDebugTrace::None,
		RawHitResults,
		true,
		FLinearColor::Red,
		FLinearColor::Green,
		1.f
	);

	if (!bHit)
	{
		return {};
	}

	TArray<ANexusCharacterBase*> HitCharacters;
	UNexusCombatFunctionLibrary::FilterHitResultsToLivingEnemyCharacters(
		this,
		RawHitResults,
		HitCharacters,
		OutValidHitResults);

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
	if (!EffectSet || !IsValid(Target) || Damage <= 0.f)
	{
		return;
	}

	const float EffectDamage = -Damage;

	ApplySetByCallerEffectToTarget(
		Target,
		EffectSet->DamageEffect,
		EffectDamage,
		NexusGameplayTags::Data_Value_Damage
	);
}

bool ANexusCharacterBase::CanParryMeleeHit(ANexusCharacterBase* Attacker) const
{
	return UNexusCombatFunctionLibrary::CanCharacterDeflectMeleeHit(
		this,
		Attacker,
		DeflectMinDotThreshold,
		NexusGameplayTags::Status_Defense_Deflecting);
}

bool ANexusCharacterBase::IsAttackWithinDeflectAngle(
	ANexusCharacterBase* Attacker,
	float MinDotThreshold) const
{
	return UNexusCombatFunctionLibrary::IsWithinFacingAngle(this, Attacker, MinDotThreshold);
}

void ANexusCharacterBase::HandleSuccessfulDeflect(
	ANexusCharacterBase* Attacker,
	const FHitResult& HitResult)
{
	if (!HasAuthority() || !IsValid(Attacker) || Attacker == this)
	{
		return;
	}

	ApplyStunToTarget(Attacker, DeflectStunDuration);

	if (bPlayDeflectCueOnDefender)
	{
		const FGameplayCueParameters DefenderParams =
			UNexusCombatFunctionLibrary::MakeImpactCueParameters(HitResult, Attacker, Attacker);

		UGameplayCueFunctionLibrary::ExecuteGameplayCueOnActor(
			this,
			NexusGameplayTags::GameplayCue_Damage_Deflected,
			DefenderParams);
	}

	if (bPlayDeflectCueOnAttacker)
	{
		const FGameplayCueParameters AttackerParams =
			UNexusCombatFunctionLibrary::MakeImpactCueParameters(HitResult, this, this);

		UGameplayCueFunctionLibrary::ExecuteGameplayCueOnActor(
			Attacker,
			NexusGameplayTags::GameplayCue_Damage_Deflected,
			AttackerParams);
	}
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

	if (!UNexusCombatFunctionLibrary::IsCharacterAlive(Target))
	{
		return ENexusHitResolutionResult::None;
	}

	if (Target->CanParryMeleeHit(this))
	{
		Target->HandleSuccessfulDeflect(this, HitResult);
		return ENexusHitResolutionResult::Parried;
	}

	ApplyDamageToTarget(Target, Damage);

	const FGameplayCueParameters Parameters =
		UNexusCombatFunctionLibrary::MakeImpactCueParameters(HitResult, this, this);

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
			if (PC->GetPawn() == this)
			{
				DisableInput(PC);
			}
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
	case ENexusAbilitySource::Class:
		return ClassAbilityHandles;
	case ENexusAbilitySource::Weapon:
		return WeaponAbilityHandles;
	default:
		return ClassAbilityHandles;
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
	const TArray<FNexusAbilityGrant>& AbilityGrants,
	UObject* SourceObject)
{
	TArray<FGameplayAbilitySpecHandle> NewHandles;

	if (!HasAuthority() || !AbilitySystemComponent)
	{
		return NewHandles;
	}

	TArray<FGameplayAbilitySpecHandle>& HandleArray = GetHandleArrayForSource(Source);

	for (const FNexusAbilityGrant& Grant : AbilityGrants)
	{
		const TSubclassOf<UNexusGameplayAbility> AbilityClass = Grant.Ability;
		if (!AbilityClass || HasGrantedAbilityClass(AbilityClass))
		{
			continue;
		}

		const UNexusGameplayAbility* AbilityCDO = AbilityClass->GetDefaultObject<UNexusGameplayAbility>();
		if (!AbilityCDO)
		{
			continue;
		}

		FGameplayAbilitySpec Spec(
			AbilityClass,
			FMath::Max(1, Grant.AbilityLevel),
			INDEX_NONE,
			SourceObject ? SourceObject : this);

		if (Grant.InputTag.IsValid())
		{
			Spec.GetDynamicSpecSourceTags().AddTag(Grant.InputTag);
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
	ClearAbilitySet(ENexusAbilitySource::Class);
	ClearAbilitySet(ENexusAbilitySource::Weapon);
}