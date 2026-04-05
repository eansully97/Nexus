#include "Nexus/Weapons/NexusWeaponBase.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Nexus/Character/NexusCharacterBase.h"

ANexusWeaponBase::ANexusWeaponBase()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
	SetReplicateMovement(true);

	SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent"));
	SetRootComponent(SceneComponent);

	WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(SceneComponent);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	OffHandWeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("OffHandWeaponMesh"));
	OffHandWeaponMesh->SetupAttachment(SceneComponent);
	OffHandWeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ANexusWeaponBase::SetOwnerCharacter(ANexusCharacterBase* InOwnerCharacter)
{
	OwnerCharacter = InOwnerCharacter;

	if (InOwnerCharacter)
	{
		SetOwner(InOwnerCharacter);
		SetInstigator(Cast<APawn>(InOwnerCharacter));
	}
}

bool ANexusWeaponBase::HasOffHandWeapon() const
{
	return OffHandWeaponMesh && OffHandWeaponMesh->GetStaticMesh() != nullptr;
}