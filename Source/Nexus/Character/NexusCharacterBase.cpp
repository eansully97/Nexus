// Fill out your copyright notice in the Description page of Project Settings.


#include "NexusCharacterBase.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Components/CapsuleComponent.h"
#include "Enemy/Minions/NexusMinionBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "Nexus/Components/NexusWeaponsManager.h"
#include "Nexus/GameMode/NexusGameMode.h"
#include "Nexus/GameplayAbilitySystem/AbilitySystemComponent/NexusAbilitySystemComponent.h"
#include "Nexus/GameplayAbilitySystem/AttributeSet/BasicAttributeSet.h"
#include "Nexus/HUD/NexusHUD.h"
#include "Nexus/PlayerState/NexusPlayerState.h"
#include "Nexus/Weapons/NexusWeaponBase.h"

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
}

void ANexusCharacterBase::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	SyncTeamFromPlayerState();

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
		if (StartingAbilities.Num() > 0)
		{
			GrantAbilities(StartingAbilities);
		}
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
	GetCapsuleComponent()->SetCollisionObjectType(ECC_WorldStatic);
	GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	
	GetMesh()->SetCollisionObjectType(ECC_WorldStatic);
	GetMesh()->SetCollisionResponseToAllChannels(ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	
	RemoveAbilities(GrantedAbilitySpecHandles);
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
	TArray<TSubclassOf<UGameplayAbility>> AbilitiesToGrant)
{
	if (!AbilitySystemComponent || !HasAuthority())
	{
		return TArray<FGameplayAbilitySpecHandle>();
	}

	for (const TSubclassOf<UGameplayAbility> Ability : AbilitiesToGrant)
	{
		FGameplayAbilitySpecHandle SpecHandle = AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(Ability, 1, -1, this));
		GrantedAbilitySpecHandles.Add(SpecHandle);
	}
	SendAbilitiesChangedEvent();
	return GrantedAbilitySpecHandles;
}

void ANexusCharacterBase::RemoveAbilities(TArray<FGameplayAbilitySpecHandle> SpecHandles)
{
	if (!AbilitySystemComponent || !HasAuthority())
	{
		return;
	}

	for (FGameplayAbilitySpecHandle SpecHandle : SpecHandles)
	{
		AbilitySystemComponent->ClearAbility(SpecHandle);
	}
	SendAbilitiesChangedEvent();
}

void ANexusCharacterBase::SendAbilitiesChangedEvent()
{
	FGameplayEventData EventData;
	EventData.EventTag = FGameplayTag::RequestGameplayTag(FName("Event.Abilities.Changed"));
	EventData.Instigator = this;
	EventData.Target = this;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, EventData.EventTag, EventData);
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

