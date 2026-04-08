#include "Nexus/Components/NexusWeaponsManager.h"

#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "Nexus/Character/Player/NexusPlayerCharacter.h"
#include "Nexus/Weapons/NexusWeaponBase.h"

UNexusWeaponsManager::UNexusWeaponsManager()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UNexusWeaponsManager::BeginPlay()
{
	Super::BeginPlay();
	OwnerCharacter = Cast<ANexusPlayerCharacter>(GetOwner());
}

void UNexusWeaponsManager::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ThisClass, EquippedWeapon);
}

void UNexusWeaponsManager::OnRep_EquippedWeapon()
{
	if (EquippedWeapon)
	{
		ApplyEquippedWeaponState();
	}
	else
	{
		SetUnarmedProperties();
	}
}

void UNexusWeaponsManager::Equip(TSubclassOf<ANexusWeaponBase> WeaponClassToEquip)
{
	EquipOrSwap(WeaponClassToEquip);
}

void UNexusWeaponsManager::EquipOrSwap(TSubclassOf<ANexusWeaponBase> WeaponClassToEquip)
{
	ANexusPlayerCharacter* Character = Cast<ANexusPlayerCharacter>(GetOwner());
	if (!Character || !Character->HasAuthority() || !WeaponClassToEquip)
	{
		return;
	}

	if (EquippedWeapon && EquippedWeapon->GetClass() == WeaponClassToEquip)
	{
		OwnerCharacter = Character;
		ApplyEquippedWeaponState();
		return;
	}

	UnequipCurrentWeapon();

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

	Weapon->SetOwnerCharacter(Character);
	Weapon->FinishSpawning(SpawnTransform);

	EquippedWeapon = Weapon;
	OwnerCharacter = Character;

	ApplyEquippedWeaponState();
}

void UNexusWeaponsManager::UnequipCurrentWeapon()
{
	ANexusPlayerCharacter* Character = Cast<ANexusPlayerCharacter>(GetOwner());
	if (!Character || !Character->HasAuthority())
	{
		return;
	}

	ANexusWeaponBase* OldWeapon = EquippedWeapon;
	EquippedWeapon = nullptr;

	if (OldWeapon)
	{
		OldWeapon->Destroy();
	}

	SetUnarmedProperties();
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

void UNexusWeaponsManager::SetEquippedWeaponProperties() const
{
	if (!OwnerCharacter || !EquippedWeapon)
	{
		return;
	}

	if (EquippedWeapon->WeaponConfig.AnimInstanceClass)
	{
		OwnerCharacter->GetMesh()->SetAnimInstanceClass(EquippedWeapon->WeaponConfig.AnimInstanceClass);
	}
	else
	{
		OwnerCharacter->GetMesh()->SetAnimInstanceClass(DefaultAnimInstanceClass);
	}

	if (UCharacterMovementComponent* MoveComp = OwnerCharacter->GetCharacterMovement())
	{
		MoveComp->bOrientRotationToMovement = false;
		MoveComp->bUseControllerDesiredRotation = true;
	}
}

void UNexusWeaponsManager::SetUnarmedProperties() const
{
	if (!OwnerCharacter)
	{
		return;
	}

	OwnerCharacter->GetMesh()->SetAnimInstanceClass(DefaultAnimInstanceClass);

	if (UCharacterMovementComponent* MoveComp = OwnerCharacter->GetCharacterMovement())
	{
		MoveComp->bOrientRotationToMovement = true;
		MoveComp->bUseControllerDesiredRotation = false;
	}
}
