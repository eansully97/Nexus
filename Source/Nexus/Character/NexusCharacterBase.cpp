#include "NexusCharacterBase.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
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
	
	if (HasAuthority())
	{
		InitializeCombatLoadout();
	}
	SetOwner(NewController);
	
	InitializeAbilityActorInfo();
	InitializeFromPlayerState();
	
}

void ANexusCharacterBase::OnRep_Controller()
{
	Super::OnRep_Controller();

	if (Controller)
	{
		SetOwner(Controller);
	}
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
	HandleDeathStateChanged();
	ForceNetUpdate();
}

void ANexusCharacterBase::ApplyDeathState_Server()
{
	if (AbilitySystemComponent)
	{
		FGameplayTagContainer StatusTags;
		StatusTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Status.Dead")));
		UAbilitySystemBlueprintLibrary::AddGameplayTags(this, StatusTags, EGameplayTagReplicationState::TagOnly);
	}
}

void ANexusCharacterBase::OnRep_IsDead()
{
	HandleDeathStateChanged();
}

void ANexusCharacterBase::HandleDeathStateChanged()
{
	ApplyDeathPresentation();
	OnCombatStateChanged.Broadcast();
}

void ANexusCharacterBase::ApplyDeathPresentation()
{
	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->StopMovementImmediately();

	if (DeathMontage)
	{
		PlayAnimMontage(DeathMontage);
	}

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

		FGameplayAbilitySpec Spec(AbilityClass, 1, INDEX_NONE, this);
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