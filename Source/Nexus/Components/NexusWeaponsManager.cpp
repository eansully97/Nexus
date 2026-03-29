// Fill out your copyright notice in the Description page of Project Settings.


#include "NexusWeaponsManager.h"

#include "CharacterClassComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "Nexus/Character/NexusCharacterBase.h"
#include "Nexus/Character/Player/NexusPlayerCharacter.h"
#include "Nexus/DataAssets/CharacterClassInfo.h"
#include "Nexus/Weapons/NexusWeaponBase.h"


UNexusWeaponsManager::UNexusWeaponsManager()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	OwnerCharacter = Cast<ANexusPlayerCharacter>(GetOwner());
	
}

void UNexusWeaponsManager::BeginPlay()
{
	Super::BeginPlay();
	OwnerCharacter = Cast<ANexusPlayerCharacter>(GetOwner());
}

void UNexusWeaponsManager::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ThisClass, EquippedWeapon)
}

void UNexusWeaponsManager::OnRep_EquippedWeapon()
{
	if (EquippedWeapon)
	{
		SetEquippedWeaponProperties();
	}
}

void UNexusWeaponsManager::Equip(TSubclassOf<ANexusWeaponBase> WeaponClassToEquip)
{
	ANexusPlayerCharacter* Character = Cast<ANexusPlayerCharacter>(GetOwner());
	if (!Character || !Character->HasAuthority() || !WeaponClassToEquip)
	{
		return;
	}

	if (EquippedWeapon)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = Character;
	SpawnParameters.Instigator = Character;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	const FTransform SpawnTransform = Character->GetActorTransform();

	ANexusWeaponBase* Weapon = World->SpawnActorDeferred<ANexusWeaponBase>(
		WeaponClassToEquip,
		SpawnTransform,
		Character,
		Character,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn
	);

	if (!Weapon)
	{
		return;
	}

	// Put any required initialization here before FinishSpawning.
	Weapon->SetOwner(Character);

	Weapon->FinishSpawning(SpawnTransform);

	EquippedWeapon = Weapon;

	FWeaponConfig WeaponConfig = Weapon->WeaponConfig;

	EquippedWeapon->AttachToComponent(
		Character->GetMesh(),
		FAttachmentTransformRules::SnapToTargetIncludingScale,
		WeaponConfig.SocketName
	);

	SetEquippedWeaponProperties();
}

void UNexusWeaponsManager::SetEquippedWeaponProperties()
{
	if (!OwnerCharacter || !EquippedWeapon)
	{
		return;
	}
	
	OwnerCharacter->GetMesh()->SetAnimInstanceClass(EquippedWeapon->WeaponConfig.AnimInstanceClass);
	OwnerCharacter->GetCharacterMovement()->bOrientRotationToMovement = false;
	OwnerCharacter->GetCharacterMovement()->bUseControllerDesiredRotation = true;
}
