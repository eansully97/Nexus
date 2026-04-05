#include "Nexus/Character/NexusCharacterBase.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
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

void ANexusCharacterBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();
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
	if (!HasAuthority())
	{
		return;
	}

	RebuildCombatLoadout();
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

void ANexusCharacterBase::Die()
{
	if (!HasAuthority() || bIsDead)
	{
		return;
	}

	if (GetCurrentMontage())
	{
		StopAnimMontage(GetCurrentMontage());
	}

	bIsDead = true;
	ApplyDeathState_Server();
	Multicast_DeathAnimation();
	HandleDeathStateChanged();
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

void ANexusCharacterBase::ApplyDeathState_Server()
{
	ClearAllGrantedAbilitySets();
	SetCanBeDamaged(false);

	if (CurrentCapturePoint)
	{
		SetCurrentCapturePoint(nullptr);
	}

	if (DeathLifeSpan > 0.0f)
	{
		SetLifeSpan(DeathLifeSpan);
	}
}

void ANexusCharacterBase::ApplyDeathPresentation()
{
	DisableCharacterForDeath();

	BP_OnDeathStarted();
}

void ANexusCharacterBase::Multicast_DeathAnimation_Implementation()
{
	PlayDeathAnimation();
}

void ANexusCharacterBase::PlayDeathAnimation()
{
	if (DeathMontage && GetMesh())
	{
		PlayAnimMontage(DeathMontage);
	}
}

void ANexusCharacterBase::EnterRagdoll()
{
	DisableCharacterForDeath();
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->SetCollisionProfileName(TEXT("Ragdoll"));
		MeshComp->SetAllBodiesSimulatePhysics(true);
		MeshComp->SetSimulatePhysics(true);
		MeshComp->bBlendPhysics = true;
	}
}

void ANexusCharacterBase::DisableCharacterForDeath()
{
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->StopMovementImmediately();
		MoveComp->DisableMovement();
	}
	
	if (UCapsuleComponent* CapsuleComp = GetCapsuleComponent())
	{
		CapsuleComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	}
}

TArray<FGameplayAbilitySpecHandle> ANexusCharacterBase::GrantAbilitySet(
	ENexusAbilitySource Source,
	const TArray<FNexusAbilityGrant>& AbilityGrants,
	UObject* SourceObject)
{
	TArray<FGameplayAbilitySpecHandle> GrantedHandles;

	if (!HasAuthority() || !AbilitySystemComponent)
	{
		return GrantedHandles;
	}

	TArray<FGameplayAbilitySpecHandle>& HandleArray = GetHandleArrayForSource(Source);

	for (const FNexusAbilityGrant& Grant : AbilityGrants)
	{
		if (!Grant.Ability)
		{
			continue;
		}

		if (HasGrantedAbilityClass(Grant.Ability))
		{
			continue;
		}

		FGameplayAbilitySpec Spec(Grant.Ability, Grant.AbilityLevel);
		Spec.SourceObject = SourceObject;

		if (Grant.InputTag.IsValid())
		{
			Spec.GetDynamicSpecSourceTags().AddTag(Grant.InputTag);
		}

		const FGameplayAbilitySpecHandle Handle = AbilitySystemComponent->GiveAbility(Spec);
		if (Handle.IsValid())
		{
			HandleArray.Add(Handle);
			GrantedHandles.Add(Handle);
		}
	}

	return GrantedHandles;
}

void ANexusCharacterBase::ClearAbilitySet(ENexusAbilitySource Source)
{
	if (!HasAuthority() || !AbilitySystemComponent)
	{
		return;
	}

	TArray<FGameplayAbilitySpecHandle>& HandleArray = GetHandleArrayForSource(Source);

	for (const FGameplayAbilitySpecHandle& Handle : HandleArray)
	{
		if (Handle.IsValid())
		{
			AbilitySystemComponent->ClearAbility(Handle);
		}
	}

	HandleArray.Reset();
}

void ANexusCharacterBase::ClearAllGrantedAbilitySets()
{
	ClearAbilitySet(ENexusAbilitySource::Class);
	ClearAbilitySet(ENexusAbilitySource::Weapon);
}

TArray<FGameplayAbilitySpecHandle>& ANexusCharacterBase::GetHandleArrayForSource(ENexusAbilitySource Source)
{
	switch (Source)
	{
	case ENexusAbilitySource::Weapon:
		return WeaponAbilityHandles;

	case ENexusAbilitySource::Class:
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

	TArray<FGameplayAbilitySpecHandle> Handles;
	AbilitySystemComponent->GetAllAbilities(Handles);

	for (const FGameplayAbilitySpecHandle& Handle : Handles)
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

	TArray<FGameplayAbilitySpecHandle> Handles;
	AbilitySystemComponent->GetAllAbilities(Handles);

	for (const FGameplayAbilitySpecHandle& Handle : Handles)
	{
		const FGameplayAbilitySpec* Spec = AbilitySystemComponent->FindAbilitySpecFromHandle(Handle);
		const UNexusGameplayAbility* AbilityCDO = Spec ? Cast<UNexusGameplayAbility>(Spec->Ability) : nullptr;

		if (!AbilityCDO)
		{
			continue;
		}

		if (AbilityCDO->GetAssetTags().HasTagExact(AbilityTag))
		{
			return true;
		}
	}

	return false;
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

	ClearAllGrantedAbilitySets();

	const TArray<FNexusAbilityGrant> ClassGrants = GetClassAbilitiesToGrant();
	if (ClassGrants.Num() > 0)
	{
		GrantAbilitySet(ENexusAbilitySource::Class, ClassGrants, this);
	}

	OnCombatStateChanged.Broadcast();
}

void ANexusCharacterBase::DebugLogGrantedAbilities(const FString& Context) const
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	TArray<FGameplayAbilitySpecHandle> Handles;
	AbilitySystemComponent->GetAllAbilities(Handles);

	UE_LOG(LogTemp, Warning, TEXT("[%s] Granted ability count: %d"), *Context, Handles.Num());
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

	UKismetSystemLibrary::SphereTraceMultiForObjects(
		GetWorld(),
		StartLocation,
		EndLocation,
		Radius,
		ObjectTypes,
		false,
		ActorsToIgnore,
		EDrawDebugTrace::None,
		RawHitResults,
		true);

	TArray<ANexusCharacterBase*> HitCharacters;

	for (const FHitResult& Hit : RawHitResults)
	{
		ANexusCharacterBase* HitCharacter = Cast<ANexusCharacterBase>(Hit.GetActor());
		if (!HitCharacter || !IsEnemyTo(HitCharacter) || HitCharacter->GetIsDead())
		{
			continue;
		}

		HitCharacters.AddUnique(HitCharacter);
		OutValidHitResults.Add(Hit);
	}

	return HitCharacters;
}

ENexusHitResolutionResult ANexusCharacterBase::ResolveMeleeHit(
	ANexusCharacterBase* Target,
	const FHitResult& HitResult,
	float Damage)
{
	if (!Target || Target->GetIsDead())
	{
		return ENexusHitResolutionResult::None;
	}

	const FVector ToAttacker = (GetActorLocation() - Target->GetActorLocation()).GetSafeNormal2D();
	const FVector Forward = Target->GetActorForwardVector().GetSafeNormal2D();
	const float Dot = FVector::DotProduct(Forward, ToAttacker);

	if (UNexusCombatFunctionLibrary::CanCharacterDeflectMeleeHit(this, Target, Dot, NexusGameplayTags::Status_Defense_Deflecting))
	{
		Target->HandleSuccessfulDeflect(this, HitResult);
		return ENexusHitResolutionResult::Parried;
	}

	ApplyDamageToTargetWithCueParams(Target, Damage, this, this, &HitResult);
	return ENexusHitResolutionResult::Damaged;
}

void ANexusCharacterBase::HandleSuccessfulDeflect(
	ANexusCharacterBase* Attacker,
	const FHitResult& HitResult)
{
	if (!Attacker)
	{
		return;
	}

	if (bPlayDeflectCueOnDefender)
	{
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
			this,
			NexusGameplayTags::Event_Deflect_Triggered,
			FGameplayEventData());
	}

	if (bPlayDeflectCueOnAttacker)
	{
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
			Attacker,
			NexusGameplayTags::Event_Deflect_Triggered,
			FGameplayEventData());
	}

	ApplyStunToTarget(Attacker, DeflectStunDuration);
}

void ANexusCharacterBase::ApplyGameplayEffectSpecsToTarget(
	const TArray<FGameplayEffectSpecHandle>& EffectSpecHandles,
	ANexusCharacterBase* TargetToEffect)
{
	if (!TargetToEffect || !TargetToEffect->GetAbilitySystemComponent())
	{
		return;
	}

	for (const FGameplayEffectSpecHandle& SpecHandle : EffectSpecHandles)
	{
		if (SpecHandle.IsValid() && SpecHandle.Data.IsValid())
		{
			TargetToEffect->GetAbilitySystemComponent()->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
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
	if (!AbilitySystemComponent || !Target || !Target->GetAbilitySystemComponent() || !EffectClass || !DataTag.IsValid())
	{
		return;
	}

	FGameplayEffectContextHandle ContextHandle = AbilitySystemComponent->MakeEffectContext();
	ContextHandle.AddSourceObject(this);

	FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(EffectClass, 1.f, ContextHandle);
	if (!SpecHandle.IsValid() || !SpecHandle.Data.IsValid())
	{
		return;
	}

	SpecHandle.Data->SetSetByCallerMagnitude(DataTag, Magnitude);
	Target->GetAbilitySystemComponent()->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
}

void ANexusCharacterBase::ApplyStunToTarget(
	ANexusCharacterBase* Target,
	float Duration)
{
	if (!EffectSet || !Target)
	{
		return;
	}

	ApplySetByCallerEffectToTarget(
		Target,
		EffectSet->StunEffect,
		Duration,
		NexusGameplayTags::Data_Value_Duration);
}

void ANexusCharacterBase::ApplyDamageToTarget(
	ANexusCharacterBase* Target,
	float Damage)
{
	if (!EffectSet || !Target)
	{
		return;
	}

	Damage = -FMath::Abs(Damage);

	ApplySetByCallerEffectToTarget(
		Target,
		EffectSet->DamageEffect,
		Damage,
		NexusGameplayTags::Data_Value_Damage);
}

void ANexusCharacterBase::ApplyDamageToTargetWithCueParams(
	ANexusCharacterBase* Target,
	float Damage,
	AActor* InstigatorActor,
	AActor* EffectCauserActor,
	const FHitResult* HitResult,
	UObject* SourceObject)
{
	if (!EffectSet || !EffectSet->DamageEffect || !IsValid(Target))
	{
		return;
	}

	UAbilitySystemComponent* SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(this);
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);

	if (!SourceASC || !TargetASC)
	{
		return;
	}

	FGameplayEffectContextHandle EffectContext = SourceASC->MakeEffectContext();
	EffectContext.AddInstigator(this, EffectCauserActor ? EffectCauserActor : this);

	if (SourceObject)
	{
		EffectContext.AddSourceObject(SourceObject);
	}

	if (InstigatorActor)
	{
		EffectContext.AddInstigator(InstigatorActor, EffectCauserActor);
	}

	if (HitResult)
	{
		EffectContext.AddHitResult(*HitResult, true);
	}

	if (HitResult)
	{
		EffectContext.AddOrigin(HitResult->ImpactPoint);
	}

	FGameplayEffectSpecHandle SpecHandle =
		SourceASC->MakeOutgoingSpec(EffectSet->DamageEffect, 1.0f, EffectContext);

	if (!SpecHandle.IsValid() || !SpecHandle.Data.IsValid())
	{
		return;
	}

	SpecHandle.Data->SetSetByCallerMagnitude(
		NexusGameplayTags::Data_Value_Damage,
		-FMath::Abs(Damage));

	SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
}