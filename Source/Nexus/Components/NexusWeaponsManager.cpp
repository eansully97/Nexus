// Fill out your copyright notice in the Description page of Project Settings.


#include "NexusWeaponsManager.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "Nexus/Character/NexusCharacterBase.h"
#include "Nexus/Weapons/NexusWeaponBase.h"


UNexusWeaponsManager::UNexusWeaponsManager()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UNexusWeaponsManager::BeginPlay()
{
	Super::BeginPlay();
	
	OwnerCharacter = Cast<ANexusCharacterBase>(GetOwner());
	
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
	else
	{
		SetUnarmedProperties();
	}
}

void UNexusWeaponsManager::Equip(TSubclassOf<ANexusWeaponBase> WeaponClassToEquip)
{
	if (!OwnerCharacter || !OwnerCharacter->HasAuthority() || !WeaponClassToEquip)
	{
		return;
	}

	if (EquippedWeapon)
	{
		return;
	}
	
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = OwnerCharacter;
	SpawnParameters.Instigator = OwnerCharacter;
	
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	ANexusWeaponBase* Weapon = World->SpawnActor<ANexusWeaponBase>(
		WeaponClassToEquip,
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		SpawnParameters
	);

	if (!Weapon)
	{
		return;
	}
	EquippedWeapon = Weapon;
	Weapon->SetOwnerCharacter(OwnerCharacter);
		
	const FName SocketName = EquippedWeapon->WeaponData.SocketName;
	EquippedWeapon->AttachToComponent(
		OwnerCharacter->GetMesh(),
		FAttachmentTransformRules::SnapToTargetIncludingScale,
		SocketName
		);
	
	WeaponAbilities = OwnerCharacter->GrantAbilities(
	EquippedWeapon->WeaponData.AbilitiesToGrant,
	false,
	true
	);
	
	SetEquippedWeaponProperties();
}

void UNexusWeaponsManager::Unequip()
{
	if (!EquippedWeapon || !OwnerCharacter || !OwnerCharacter->HasAuthority())
	{
		return;
	}

	ANexusWeaponBase* OldWeapon = EquippedWeapon;
	EquippedWeapon = nullptr;

	if (WeaponAbilities.Num() > 0)
	{
		OwnerCharacter->RemoveAbilities(WeaponAbilities);
		WeaponAbilities.Empty();
	}

	if (OldWeapon)
	{
		OldWeapon->SetOwnerCharacter(nullptr);
		OldWeapon->Destroy();
	}

	SetUnarmedProperties();
}

void UNexusWeaponsManager::Server_Unequip_Implementation()
{
	Unequip();
}

void UNexusWeaponsManager::SetEquippedWeaponProperties() const
{
	if (!EquippedWeapon || !OwnerCharacter)
	{
		return;
	}
	OwnerCharacter->GetMesh()->SetAnimInstanceClass(EquippedWeapon->WeaponData.AnimInstanceClass);
	OwnerCharacter->GetCharacterMovement()->bOrientRotationToMovement = false;
	OwnerCharacter->GetCharacterMovement()->bUseControllerDesiredRotation = true;
}

void UNexusWeaponsManager::SetUnarmedProperties() const
{
	if (!DefaultAnimInstanceClass || !OwnerCharacter || OwnerCharacter->bIsDead)
	{
		return;
	}
	OwnerCharacter->GetMesh()->SetAnimInstanceClass(DefaultAnimInstanceClass);
	OwnerCharacter->GetCharacterMovement()->bOrientRotationToMovement = true;
	OwnerCharacter->GetCharacterMovement()->bUseControllerDesiredRotation = false;
}
