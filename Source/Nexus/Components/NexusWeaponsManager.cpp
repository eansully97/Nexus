// Fill out your copyright notice in the Description page of Project Settings.


#include "NexusWeaponsManager.h"

#include "CharacterClassComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
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
	ApplyEquippedWeaponState();
}

void UNexusWeaponsManager::ApplyEquippedWeaponState()
{
	if (!OwnerCharacter)
	{
		OwnerCharacter = Cast<ANexusPlayerCharacter>(GetOwner());
	}

	if (!OwnerCharacter || !EquippedWeapon)
	{
		return;
	}

	AttachEquippedWeapon();
	SetEquippedWeaponProperties();
}

void UNexusWeaponsManager::AttachEquippedWeapon()
{
	if (!OwnerCharacter || !EquippedWeapon)
	{
		return;
	}

	const FWeaponConfig& WeaponConfig = EquippedWeapon->WeaponConfig;

	if (UStaticMeshComponent* WeaponMesh = EquippedWeapon->GetWeaponMesh())
	{
		WeaponMesh->AttachToComponent(
			OwnerCharacter->GetMesh(),
			FAttachmentTransformRules::SnapToTargetIncludingScale,
			WeaponConfig.WeaponSocketName
		);

		WeaponMesh->SetVisibility(true, true);
		WeaponMesh->SetHiddenInGame(false, true);
	}

	if (EquippedWeapon->HasOffHandWeapon())
	{
		if (UStaticMeshComponent* OffHandMesh = EquippedWeapon->GetOffHandWeaponMesh())
		{
			OffHandMesh->AttachToComponent(
				OwnerCharacter->GetMesh(),
				FAttachmentTransformRules::SnapToTargetIncludingScale,
				WeaponConfig.OffHandSocketName
			);

			OffHandMesh->SetVisibility(true, true);
			OffHandMesh->SetHiddenInGame(false, true);
		}
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

	Weapon->SetOwner(Character);
	Weapon->FinishSpawning(SpawnTransform);

	EquippedWeapon = Weapon;
	OwnerCharacter = Character;

	ApplyEquippedWeaponState();

	const TArray<FGameplayAbilitySpecHandle> GrantedHandles = Character->GrantAbilitySet(
		ENexusAbilitySource::Weapon,
		EquippedWeapon->WeaponConfig.AbilitiesToGrant,
		EquippedWeapon);

	UE_LOG(LogTemp, Warning,
		TEXT("Equip Weapon=%s GrantedWeaponAbilities=%d"),
		*GetNameSafe(EquippedWeapon),
		GrantedHandles.Num());
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
