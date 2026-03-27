// Fill out your copyright notice in the Description page of Project Settings.


#include "NexusCharacterBase.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "Nexus/Components/NexusWeaponsManager.h"
#include "Nexus/GameMode/NexusGameMode.h"
#include "Nexus/GameplayAbilitySystem/Abilities/NexusGameplayAbility.h"
#include "Nexus/GameplayAbilitySystem/AbilitySystemComponent/NexusAbilitySystemComponent.h"
#include "Nexus/GameplayAbilitySystem/AttributeSet/BasicAttributeSet.h"
#include "Nexus/PlayerState/NexusPlayerState.h"

ANexusCharacterBase::ANexusCharacterBase()
{
	PrimaryActorTick.bCanEverTick = true;

	AbilitySystemComponent = CreateDefaultSubobject<UNexusAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(ReplicationMode);

	BasicAttributeSet = CreateDefaultSubobject<UBasicAttributeSet>(TEXT("BasicAttributeSet"));

	WeaponsManager = CreateDefaultSubobject<UNexusWeaponsManager>( TEXT("WeaponsManager"));
	WeaponsManager->SetIsReplicated(true);

	GetCapsuleComponent()->InitCapsuleSize(35.f, 90.f);
	

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate.Yaw = 500.0f;

	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.f;
}

void ANexusCharacterBase::BeginPlay()
{
	Super::BeginPlay();
}

void ANexusCharacterBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ANexusCharacterBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void ANexusCharacterBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ANexusCharacterBase, bIsDead);
	DOREPLIFETIME(ANexusCharacterBase, TeamID);
}

void ANexusCharacterBase::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	SetOwner(NewController);
	SyncTeamFromPlayerState();

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
		AbilitySystemComponent->RefreshAbilityActorInfo();
	}

	if (HasAuthority() && StartingAbilities.Num() > 0 && StartupAbilitySpecHandles.Num() == 0)
	{
		GrantAbilities(StartingAbilities, true, false);
	}
}

void ANexusCharacterBase::OnRep_Controller()
{
	Super::OnRep_Controller();

	if (Controller)
	{
		SetOwner(Controller);
	}

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
		AbilitySystemComponent->RefreshAbilityActorInfo();
	}
}

void ANexusCharacterBase::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	SyncTeamFromPlayerState();

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}
}

void ANexusCharacterBase::SyncTeamFromPlayerState()
{
	ANexusPlayerState* NexusPS = GetPlayerState<ANexusPlayerState>();
	if (NexusPS)
	{
		SetTeamID(NexusPS->GetTeamID());
	}
}

void ANexusCharacterBase::Die()
{
	if (bIsDead)
	{
		return;
	}

	if (!HasAuthority())
	{
		return;
	}

	if (WeaponsManager && WeaponsManager->EquippedWeapon)
	{
		WeaponsManager->Unequip();
	}

	bIsDead = true;
	
	GetCharacterMovement()->DisableMovement();

	OnDeathStarted();
}

void ANexusCharacterBase::OnRep_IsDead()
{
	OnDeathStarted();
}

void ANexusCharacterBase::OnRep_TeamID()
{
	ApplyTeamVisuals();
}

ENexusTeamID ANexusCharacterBase::GetTeamID() const
{
	return TeamID;
}

void ANexusCharacterBase::SetTeamID(ENexusTeamID InTeamID)
{
	TeamID = InTeamID;
	OnRep_TeamID();
}

void ANexusCharacterBase::ApplyTeamVisuals() const
{
	
}

bool ANexusCharacterBase::IsFriendlyTo(AActor* OtherActor) const
{
	if (ANexusCharacterBase* OtherCharacter = Cast<ANexusCharacterBase>(OtherActor))
	{
		if (TeamID == OtherCharacter->TeamID)
		{
			return true;
		}
	}
	return false;
}

bool ANexusCharacterBase::IsEnemyTo(AActor* OtherActor) const
{
	if (ANexusCharacterBase* OtherCharacter = Cast<ANexusCharacterBase>(OtherActor))
	{
		if (TeamID != OtherCharacter->TeamID)
		{
			return true;
		}
	}
	return false;
}

void ANexusCharacterBase::OnDeathStarted()
{
	GetCharacterMovement()->DisableMovement();

	if (DeathMontage)
	{
		PlayAnimMontage(DeathMontage);
	}

	BP_OnDeathStarted();

	FGameplayTagContainer Container;
	Container.AddTag(FGameplayTag::RequestGameplayTag(FName("Status.Dead")));

	UAbilitySystemBlueprintLibrary::AddGameplayTags(this, Container, EGameplayTagReplicationState::TagOnly);

	GetCapsuleComponent()->SetCollisionObjectType(ECC_WorldStatic);
	GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);

	GetMesh()->SetCollisionObjectType(ECC_WorldStatic);
	GetMesh()->SetCollisionResponseToAllChannels(ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);

	if (HasAuthority())
	{
		RemoveTemporaryAbilities();
	}

	if (IsLocallyControlled())
	{
		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			DisableInput(PC);
		}
	}

	if (HasAuthority())
	{
		if (GetController() && GetController()->IsPlayerController())
		{
			if (ANexusGameMode* GM = GetWorld()->GetAuthGameMode<ANexusGameMode>())
			{
				GM->RequestRespawn(GetController(), RespawnTime);
			}
		}
	}
}

void ANexusCharacterBase::EnterRagdoll()
{
	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->StopMovementImmediately();
	

	USkeletalMeshComponent* MeshComp = GetMesh();
	if (MeshComp)
	{
		MeshComp->SetCollisionProfileName(TEXT("Ragdoll"));
		MeshComp->SetAllBodiesSimulatePhysics(true);
		MeshComp->SetSimulatePhysics(true);
		MeshComp->bBlendPhysics = true;
	}
}

UAbilitySystemComponent* ANexusCharacterBase::GetAbilitySystemComponent() const
{
	if (!AbilitySystemComponent) return nullptr;
	return AbilitySystemComponent;
}

TArray<FGameplayAbilitySpecHandle> ANexusCharacterBase::GrantAbilities(
	const TArray<TSubclassOf<UNexusGameplayAbility>>& AbilitiesToGrant,
	bool bTrackAsStartupAbilities,
	bool bTrackAsTemporaryAbilities)
{
	TArray<FGameplayAbilitySpecHandle> NewlyGrantedHandles;

	if (!AbilitySystemComponent || !HasAuthority())
	{
		return NewlyGrantedHandles;
	}

	for (const TSubclassOf<UNexusGameplayAbility>& AbilityClass : AbilitiesToGrant)
	{
		if (!AbilityClass)
		{
			continue;
		}
		
		FGameplayAbilitySpec AbilitySpec(AbilityClass, 1, INDEX_NONE, this);
		const FGameplayAbilitySpecHandle SpecHandle = AbilitySystemComponent->GiveAbility(AbilitySpec);
		
		
		if (SpecHandle.IsValid())
		{
			NewlyGrantedHandles.Add(SpecHandle);

			if (bTrackAsStartupAbilities)
			{
				StartupAbilitySpecHandles.Add(SpecHandle);
			}

			if (bTrackAsTemporaryAbilities)
			{
				TemporaryAbilitySpecHandles.Add(SpecHandle);
			}
		}
	}
	if (NewlyGrantedHandles.Num() > 0)
	{
		if (AbilitySystemComponent)
		{
			AbilitySystemComponent->ForceReplication();
		}

		ForceNetUpdate();

		BroadcastAbilitiesChanged();
		Client_NotifyAbilitiesChanged();
	}

	return NewlyGrantedHandles;
}

void ANexusCharacterBase::BroadcastAbilitiesChangedDeferred()
{
	BroadcastAbilitiesChanged();
}

void ANexusCharacterBase::RemoveAbilities(const TArray<FGameplayAbilitySpecHandle>& SpecHandles)
{
	if (!AbilitySystemComponent || !HasAuthority())
	{
		return;
	}

	bool bRemovedAtLeastOne = false;

	for (const FGameplayAbilitySpecHandle& SpecHandle : SpecHandles)
	{
		if (!SpecHandle.IsValid())
		{
			continue;
		}

		AbilitySystemComponent->ClearAbility(SpecHandle);

		StartupAbilitySpecHandles.Remove(SpecHandle);
		TemporaryAbilitySpecHandles.Remove(SpecHandle);

		bRemovedAtLeastOne = true;
	}

	if (bRemovedAtLeastOne)
	{
		if (AbilitySystemComponent)
		{
			AbilitySystemComponent->ForceReplication();
		}

		ForceNetUpdate();

		BroadcastAbilitiesChanged();
		Client_NotifyAbilitiesChanged();
	}
}

void ANexusCharacterBase::RemoveStartupAbilities()
{
	RemoveAbilities(StartupAbilitySpecHandles);
	StartupAbilitySpecHandles.Empty();
}

void ANexusCharacterBase::RemoveTemporaryAbilities()
{
	RemoveAbilities(TemporaryAbilitySpecHandles);
	TemporaryAbilitySpecHandles.Empty();
}

void ANexusCharacterBase::BroadcastAbilitiesChanged()
{
	OnAbilitiesChanged.Broadcast();
}

void ANexusCharacterBase::Client_NotifyAbilitiesChanged_Implementation()
{
	BroadcastAbilitiesChanged();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DeferredAbilitiesChangedHandle);
		World->GetTimerManager().SetTimer(
			DeferredAbilitiesChangedHandle,
			this,
			&ANexusCharacterBase::BroadcastAbilitiesChangedDeferred,
			0.05f,
			false
		);
	}
}

void ANexusCharacterBase::SetCharacterSpeedToZero()
{
	if (GetCharacterMovement())
	{
		CachedAcceleration = GetCharacterMovement()->MaxAcceleration;
		CachedJumpHeight =	GetCharacterMovement()->JumpZVelocity;
		CachedWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;
		
		GetCharacterMovement()->MaxWalkSpeed = 0;
		GetCharacterMovement()->JumpZVelocity = 0;
		GetCharacterMovement()->MaxAcceleration = 0;
	}
}

void ANexusCharacterBase::RestoreCharacterMovement()
{
	GetCharacterMovement()->MaxWalkSpeed = CachedWalkSpeed;
	GetCharacterMovement()->JumpZVelocity = CachedJumpHeight;
	GetCharacterMovement()->MaxAcceleration = CachedAcceleration;
}

ANexusCapturePoint* ANexusCharacterBase::GetCurrentCapturePoint()
{
	if (bAtCapturePoint)
	{
		return CurrentCapturePoint;
	}
	return nullptr;
}