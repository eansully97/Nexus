// NexusCharacterBase.cpp

#include "Nexus/Character/NexusCharacterBase.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Blueprint/UserWidget.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Net/UnrealNetwork.h"
#include "Nexus/NexusGameplayTags.h"
#include "Nexus/DataAssets/GameplayEffectClassSet.h"
#include "Nexus/FunctionLibraries/NexusAbilityFunctionLibrary.h"
#include "Nexus/FunctionLibraries/NexusCombatFunctionLibrary.h"
#include "Nexus/HUD/Widgets/NexusCharacterOverheadWidget.h"
#include "Nexus/GameplayAbilitySystem/Abilities/NexusGameplayAbility.h"
#include "Nexus/GameplayAbilitySystem/AbilitySystemComponent/NexusAbilitySystemComponent.h"
#include "Nexus/GameplayAbilitySystem/AttributeSet/BasicAttributeSet.h"
#include "Nexus/PlayerState/NexusPlayerState.h"

DEFINE_LOG_CATEGORY_STATIC(LogNexusCharacterBase, Log, All);

ANexusCharacterBase::ANexusCharacterBase()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	AbilitySystemComponent = CreateDefaultSubobject<UNexusAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(AbilityReplicationMode);

	BasicAttributeSet = CreateDefaultSubobject<UBasicAttributeSet>(TEXT("BasicAttributeSet"));

	GetCapsuleComponent()->InitCapsuleSize(35.f, 90.f);

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->MinAnalogWalkSpeed = 20.f;
		MoveComp->BrakingDecelerationWalking = 2000.f;
		MoveComp->BrakingDecelerationFalling = 1500.f;
	}

	OverheadWidgetClass = UNexusCharacterOverheadWidget::StaticClass();

	ApplyMovementAttributesToMovementComponent();
}

void ANexusCharacterBase::BeginPlay()
{
	Super::BeginPlay();
	BindMovementAttributeDelegates();
	ApplyMovementAttributesToMovementComponent();
	InitializeOverheadWidget();
	RefreshOverheadWidget();
}

void ANexusCharacterBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (AbilitySystemComponent)
	{
		// Re-apply here so Blueprint overrides on AbilityReplicationMode are respected.
		AbilitySystemComponent->SetReplicationMode(AbilityReplicationMode);
	}
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
	if (!AbilitySystemComponent)
	{
		return;
	}

	AbilitySystemComponent->SetReplicationMode(AbilityReplicationMode);
	AbilitySystemComponent->InitAbilityActorInfo(this, this);
	AbilitySystemComponent->RefreshAbilityActorInfo();
	BindMovementAttributeDelegates();
	ApplyMovementAttributesToMovementComponent();

	RefreshDeadGameplayTag();
}

void ANexusCharacterBase::InitializeFromPlayerState()
{
	if (const ANexusPlayerState* PS = GetPlayerState<ANexusPlayerState>())
	{
		SetTeamID(PS->GetTeamID());
	}

	RefreshOverheadWidget();
}

void ANexusCharacterBase::InitializeCombatLoadout()
{
	if (!HasAuthority() || bIsDead)
	{
		return;
	}

	RebuildCombatLoadout();
}

void ANexusCharacterBase::BindMovementAttributeDelegates()
{
	if (!AbilitySystemComponent || !BasicAttributeSet)
	{
		return;
	}

	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
		BasicAttributeSet->GetMoveSpeedAttribute()).RemoveAll(this);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
		BasicAttributeSet->GetJumpVelocityAttribute()).RemoveAll(this);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
		BasicAttributeSet->GetAirControlAttribute()).RemoveAll(this);

	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
		BasicAttributeSet->GetMoveSpeedAttribute()).AddUObject(this, &ThisClass::HandleMovementAttributeChanged);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
		BasicAttributeSet->GetJumpVelocityAttribute()).AddUObject(this, &ThisClass::HandleMovementAttributeChanged);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
		BasicAttributeSet->GetAirControlAttribute()).AddUObject(this, &ThisClass::HandleMovementAttributeChanged);
}

void ANexusCharacterBase::ApplyMovementAttributesToMovementComponent() const
{
	const UBasicAttributeSet* Attributes = BasicAttributeSet;
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (!Attributes || !MoveComp)
	{
		return;
	}

	const float MoveSpeed = FMath::Max(0.0f, Attributes->GetMoveSpeed());
	MoveComp->MaxWalkSpeed = MoveSpeed;
	MoveComp->MinAnalogWalkSpeed = FMath::Min(20.0f, MoveSpeed);
	MoveComp->JumpZVelocity = FMath::Max(0.0f, Attributes->GetJumpVelocity());
	MoveComp->AirControl = FMath::Clamp(Attributes->GetAirControl(), 0.0f, 1.0f);
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
		ForceNetUpdate();
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
	RefreshOverheadWidget();
	OnTeamChanged.Broadcast();
	BroadcastLegacyCombatStateChanged();
}

void ANexusCharacterBase::ApplyTeamVisuals() const
{
	// Intentionally left as an extension point for derived classes / blueprints.
}

bool ANexusCharacterBase::IsFriendlyTo(AActor* OtherActor) const
{
	const ANexusCharacterBase* OtherCharacter = Cast<ANexusCharacterBase>(OtherActor);
	return OtherCharacter && OtherCharacter != this && TeamID == OtherCharacter->TeamID;
}

bool ANexusCharacterBase::IsEnemyTo(AActor* OtherActor) const
{
	const ANexusCharacterBase* OtherCharacter = Cast<ANexusCharacterBase>(OtherActor);
	return OtherCharacter && OtherCharacter != this && TeamID != OtherCharacter->TeamID;
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
		ForceNetUpdate();
		HandleCurrentCapturePointChanged();
	}
}

void ANexusCharacterBase::OnRep_CurrentCapturePoint()
{
	HandleCurrentCapturePointChanged();
}

void ANexusCharacterBase::HandleCurrentCapturePointChanged()
{
	OnCapturePointChanged.Broadcast();
	BroadcastLegacyCombatStateChanged();
}

void ANexusCharacterBase::InitializeOverheadWidget()
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	if (!OverheadWidgetComponent)
	{
		TArray<UWidgetComponent*> WidgetComponents;
		GetComponents<UWidgetComponent>(WidgetComponents);

		for (UWidgetComponent* WidgetComponent : WidgetComponents)
		{
			if (!WidgetComponent)
			{
				continue;
			}

			const bool bLooksLikeOverheadWidget =
				WidgetComponent->GetName().Contains(TEXT("Overhead")) ||
				(WidgetComponent->GetWidgetClass() &&
					WidgetComponent->GetWidgetClass()->GetName().Contains(TEXT("Overhead")));

			if (bLooksLikeOverheadWidget)
			{
				OverheadWidgetComponent = WidgetComponent;
				break;
			}
		}

		if (!OverheadWidgetComponent && WidgetComponents.Num() > 0)
		{
			OverheadWidgetComponent = WidgetComponents[0];
		}
	}

	if (!OverheadWidgetComponent)
	{
		OverheadWidgetComponent = NewObject<UWidgetComponent>(this, TEXT("OverheadWidgetComponent"));
		if (!OverheadWidgetComponent)
		{
			return;
		}

		AddOwnedComponent(OverheadWidgetComponent);
		OverheadWidgetComponent->SetupAttachment(GetRootComponent());
		OverheadWidgetComponent->RegisterComponent();
	}

	OverheadWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	OverheadWidgetComponent->SetDrawAtDesiredSize(true);
	OverheadWidgetComponent->SetPivot(FVector2D(0.5f, 1.0f));
	OverheadWidgetComponent->SetRelativeLocation(OverheadWidgetRelativeLocation);
	OverheadWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	OverheadWidgetComponent->SetGenerateOverlapEvents(false);
	OverheadWidgetComponent->SetCastShadow(false);
	OverheadWidgetComponent->SetWindowFocusable(false);

	TSubclassOf<UUserWidget> DesiredWidgetClass = OverheadWidgetClass;
	if (!DesiredWidgetClass)
	{
		DesiredWidgetClass = UNexusCharacterOverheadWidget::StaticClass();
	}

	if (OverheadWidgetComponent->GetWidgetClass() != DesiredWidgetClass)
	{
		OverheadWidgetComponent->SetWidgetClass(DesiredWidgetClass);
	}

	OverheadWidgetComponent->InitWidget();
}

void ANexusCharacterBase::RefreshOverheadWidget()
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	InitializeOverheadWidget();

	if (!OverheadWidgetComponent)
	{
		return;
	}

	if (UNexusCharacterOverheadWidget* OverheadWidget =
		Cast<UNexusCharacterOverheadWidget>(OverheadWidgetComponent->GetUserWidgetObject()))
	{
		OverheadWidget->SetObservedCharacter(this);
	}
}

void ANexusCharacterBase::HandleMovementAttributeChanged(const FOnAttributeChangeData& ChangeData)
{
	ApplyMovementAttributesToMovementComponent();
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

	if (bIsDead)
	{
		ApplyDeathPresentation();
	}

	OnDeathStateChanged.Broadcast();
	BroadcastLegacyCombatStateChanged();
}

void ANexusCharacterBase::ApplyDeathState_Server()
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->CancelAllAbilities();
	}

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
		CapsuleComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
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
	if (GrantedHandles.Num() > 0)
	{
		BroadcastGrantedAbilitiesChanged();
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
	BroadcastGrantedAbilitiesChanged();
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
	if (!HasAuthority() || bIsDead)
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

	UE_LOG(LogNexusCharacterBase, Log, TEXT("[%s] Granted ability count: %d"), *Context, Handles.Num());
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

		if (HitCharacters.Contains(HitCharacter))
		{
			continue;
		}

		HitCharacters.Add(HitCharacter);
		OutValidHitResults.Add(Hit);
	}

	return HitCharacters;
}

ENexusHitResolutionResult ANexusCharacterBase::ResolveMeleeHit(
	ANexusCharacterBase* Target,
	const FHitResult& HitResult,
	float Damage)
{
	if (!Target || Target == this || Target->GetIsDead() || !IsEnemyTo(Target))
	{
		return ENexusHitResolutionResult::None;
	}

	if (UNexusCombatFunctionLibrary::CanCharacterDeflectMeleeHit(
			Target,
			this,
			DeflectMinDotThreshold,
			NexusGameplayTags::Status_Defense_Deflecting))
	{
		if (UAbilitySystemComponent* TargetASC = Target->GetAbilitySystemComponent())
		{
			TargetASC->CurrentMontageStop();
		}

		Target->HandleSuccessfulDeflect(this, HitResult);
		return ENexusHitResolutionResult::Deflected;
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

	auto BuildDeflectEventData =
		[&HitResult](AActor* EventInstigator, AActor* EventTarget, UAbilitySystemComponent* ContextASC)
		{
			FGameplayEventData EventData;
			EventData.Instigator = EventInstigator;
			EventData.Target = EventTarget;

			if (ContextASC)
			{
				FGameplayEffectContextHandle ContextHandle = ContextASC->MakeEffectContext();
				ContextHandle.AddInstigator(EventInstigator, EventInstigator);
				ContextHandle.AddHitResult(HitResult, true);
				EventData.ContextHandle = ContextHandle;
			}

			return EventData;
		};

	if (bPlayDeflectCueOnDefender)
	{
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
			this,
			NexusGameplayTags::Event_Deflect_Triggered,
			BuildDeflectEventData(Attacker, this, GetAbilitySystemComponent()));
	}

	if (bPlayDeflectCueOnAttacker)
	{
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
			Attacker,
			NexusGameplayTags::Event_Deflect_Triggered,
			BuildDeflectEventData(this, Attacker, Attacker->GetAbilitySystemComponent()));
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

bool ANexusCharacterBase::SendGameplayEventToSelf(
	const FGameplayTag& EventTag,
	AActor* TargetActor,
	AActor* OptionalObject)
{
	return UNexusAbilityFunctionLibrary::SendTargetedGameplayEventToActor(
		this,
		EventTag,
		TargetActor,
		OptionalObject);
}

void ANexusCharacterBase::ApplySetByCallerEffectToTarget(
	ANexusCharacterBase* Target,
	TSubclassOf<UGameplayEffect> EffectClass,
	float Magnitude,
	FGameplayTag DataTag)
{
	UAbilitySystemComponent* SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(this);
	UAbilitySystemComponent* TargetASC = Target ? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target) : nullptr;

	if (!SourceASC || !TargetASC || !EffectClass || !DataTag.IsValid())
	{
		return;
	}

	FGameplayEffectContextHandle ContextHandle = SourceASC->MakeEffectContext();
	ContextHandle.AddInstigator(this, this);
	ContextHandle.AddSourceObject(this);

	FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(EffectClass, 1.0f, ContextHandle);
	if (!SpecHandle.IsValid() || !SpecHandle.Data.IsValid())
	{
		return;
	}

	SpecHandle.Data->SetSetByCallerMagnitude(DataTag, Magnitude);
	SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
}

void ANexusCharacterBase::ApplyStunToTarget(
	ANexusCharacterBase* Target,
	float Duration)
{
	if (!EffectSet || !EffectSet->StunEffect || !Target || Duration <= 0.0f)
	{
		return;
	}

	ApplySetByCallerEffectToTarget(
		Target,
		EffectSet->StunEffect,
		Duration,
		NexusGameplayTags::Data_Value_Duration);
}

FGameplayEffectSpecHandle ANexusCharacterBase::BuildEffectSpec(
	const FNexusEffectApplicationParams& Params) const
{
	FGameplayEffectSpecHandle EmptyHandle;

	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponent();
	if (!SourceASC || !Params.EffectClass)
	{
		return EmptyHandle;
	}

	AActor* ResolvedInstigator = Params.InstigatorActor ? Params.InstigatorActor.Get() : const_cast<ANexusCharacterBase*>(this);
	AActor* ResolvedEffectCauser = Params.EffectCauserActor ? Params.EffectCauserActor.Get() : ResolvedInstigator;
	UObject* ResolvedSourceObject = Params.SourceObject ? Params.SourceObject.Get() : const_cast<ANexusCharacterBase*>(this);

	FGameplayEffectContextHandle EffectContext = SourceASC->MakeEffectContext();
	EffectContext.AddInstigator(ResolvedInstigator, ResolvedEffectCauser);
	EffectContext.AddSourceObject(ResolvedSourceObject);

	if (Params.bIncludeHitResult)
	{
		EffectContext.AddHitResult(Params.HitResult, true);

		if (Params.bUseHitResultImpactPointAsOrigin)
		{
			EffectContext.AddOrigin(Params.HitResult.ImpactPoint);
		}
	}

	FGameplayEffectSpecHandle SpecHandle =
		SourceASC->MakeOutgoingSpec(Params.EffectClass, Params.EffectLevel, EffectContext);

	if (!SpecHandle.IsValid() || !SpecHandle.Data.IsValid())
	{
		return EmptyHandle;
	}

	for (const FNexusSetByCallerMagnitude& MagnitudeData : Params.SetByCallerMagnitudes)
	{
		if (!MagnitudeData.DataTag.IsValid())
		{
			continue;
		}

		SpecHandle.Data->SetSetByCallerMagnitude(
			MagnitudeData.DataTag,
			MagnitudeData.Magnitude);
	}

	return SpecHandle;
}

void ANexusCharacterBase::ApplyEffectToTarget(
	ANexusCharacterBase* Target,
	const FNexusEffectApplicationParams& Params)
{
	if (!IsValid(Target))
	{
		return;
	}

	if (Params.bSkipIfTargetIsDead && Target->GetIsDead())
	{
		return;
	}

	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponent();
	UAbilitySystemComponent* TargetASC = Target->GetAbilitySystemComponent();

	if (!SourceASC || !TargetASC)
	{
		return;
	}

	const FGameplayEffectSpecHandle SpecHandle = BuildEffectSpec(Params);
	if (!SpecHandle.IsValid() || !SpecHandle.Data.IsValid())
	{
		return;
	}

	SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
}

void ANexusCharacterBase::ApplyEffectToTargets(
	const TArray<ANexusCharacterBase*>& Targets,
	const FNexusEffectApplicationParams& Params)
{
	for (ANexusCharacterBase* Target : Targets)
	{
		ApplyEffectToTarget(Target, Params);
	}
}

void ANexusCharacterBase::ApplyDamageToTarget(
	ANexusCharacterBase* Target,
	float Damage)
{
	if (!EffectSet || !EffectSet->InstantDamageEffect || Damage == 0.0f)
	{
		return;
	}

	FNexusEffectApplicationParams Params;
	Params.EffectClass = EffectSet->InstantDamageEffect;

	FNexusSetByCallerMagnitude DamageMagnitude;
	DamageMagnitude.DataTag = NexusGameplayTags::Data_Value_Damage;
	DamageMagnitude.Magnitude = -FMath::Abs(Damage);
	Params.SetByCallerMagnitudes.Add(DamageMagnitude);

	ApplyEffectToTarget(Target, Params);
}

void ANexusCharacterBase::ApplyDamageToTargetWithDuration(
	ANexusCharacterBase* Target,
	float Damage,
	float Duration)
{
	if (!EffectSet || !EffectSet->DamageWithDurationEffect || Damage == 0.0f || Duration <= 0.0f)
	{
		return;
	}

	FNexusEffectApplicationParams Params;
	Params.EffectClass = EffectSet->DamageWithDurationEffect;

	FNexusSetByCallerMagnitude DamageMagnitude;
	DamageMagnitude.DataTag = NexusGameplayTags::Data_Value_Damage;
	DamageMagnitude.Magnitude = -FMath::Abs(Damage);
	Params.SetByCallerMagnitudes.Add(DamageMagnitude);

	FNexusSetByCallerMagnitude DurationMagnitude;
	DurationMagnitude.DataTag = NexusGameplayTags::Data_Value_Duration;
	DurationMagnitude.Magnitude = Duration;
	Params.SetByCallerMagnitudes.Add(DurationMagnitude);

	ApplyEffectToTarget(Target, Params);
}

void ANexusCharacterBase::ApplyHealToTarget(
	ANexusCharacterBase* Target,
	float HealAmount)
{
	// Assumes you have a heal gameplay tag like NexusGameplayTags::Data_Value_Heal.
	if (!EffectSet || !EffectSet->InstantHealEffect || HealAmount <= 0.0f)
	{
		return;
	}

	FNexusEffectApplicationParams Params;
	Params.EffectClass = EffectSet->InstantHealEffect;

	FNexusSetByCallerMagnitude HealMagnitude;
	HealMagnitude.DataTag = NexusGameplayTags::Data_Value_Heal;
	HealMagnitude.Magnitude = FMath::Abs(HealAmount);
	Params.SetByCallerMagnitudes.Add(HealMagnitude);

	ApplyEffectToTarget(Target, Params);
}

void ANexusCharacterBase::ApplyHealToTargetWithDuration(
	ANexusCharacterBase* Target,
	float HealAmount,
	float Duration)
{
	// Assumes you have a heal gameplay tag like NexusGameplayTags::Data_Value_Heal.
	if (!EffectSet || !EffectSet->HealWithDurationEffect || HealAmount <= 0.0f || Duration <= 0.0f)
	{
		return;
	}

	FNexusEffectApplicationParams Params;
	Params.EffectClass = EffectSet->HealWithDurationEffect;

	FNexusSetByCallerMagnitude HealMagnitude;
	HealMagnitude.DataTag = NexusGameplayTags::Data_Value_Heal;
	HealMagnitude.Magnitude = FMath::Abs(HealAmount);
	Params.SetByCallerMagnitudes.Add(HealMagnitude);

	FNexusSetByCallerMagnitude DurationMagnitude;
	DurationMagnitude.DataTag = NexusGameplayTags::Data_Value_Duration;
	DurationMagnitude.Magnitude = Duration;
	Params.SetByCallerMagnitudes.Add(DurationMagnitude);

	ApplyEffectToTarget(Target, Params);
}

void ANexusCharacterBase::ApplyDamageToTargetWithCueParams(
	ANexusCharacterBase* Target,
	float Damage,
	AActor* InstigatorActor,
	AActor* EffectCauserActor,
	const FHitResult* HitResult,
	UObject* SourceObject)
{
	if (!EffectSet || !EffectSet->InstantDamageEffect || !IsValid(Target) || Damage == 0.0f)
	{
		return;
	}

	FNexusEffectApplicationParams Params;
	Params.EffectClass = EffectSet->InstantDamageEffect;
	Params.InstigatorActor = InstigatorActor ? InstigatorActor : this;
	Params.EffectCauserActor = EffectCauserActor ? EffectCauserActor : this;
	Params.SourceObject = SourceObject ? SourceObject : this;

	if (HitResult)
	{
		Params.bIncludeHitResult = true;
		Params.HitResult = *HitResult;
	}

	FNexusSetByCallerMagnitude DamageMagnitude;
	DamageMagnitude.DataTag = NexusGameplayTags::Data_Value_Damage;
	DamageMagnitude.Magnitude = -FMath::Abs(Damage);
	Params.SetByCallerMagnitudes.Add(DamageMagnitude);

	ApplyEffectToTarget(Target, Params);
}

void ANexusCharacterBase::BroadcastGrantedAbilitiesChanged()
{
	OnGrantedAbilitiesChanged.Broadcast();
	BroadcastLegacyCombatStateChanged();
}

void ANexusCharacterBase::BroadcastLegacyCombatStateChanged()
{
	OnCombatStateChanged.Broadcast();
}
