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
	if (OwnerCharacter)
	{
		OwnerCharacter->OnDeathStateChanged.RemoveDynamic(this, &ThisClass::HandleOwnerDeathStateChanged);
	}

	OwnerCharacter = InOwnerCharacter;

	if (InOwnerCharacter)
	{
		SetOwner(InOwnerCharacter);
		SetInstigator(Cast<APawn>(InOwnerCharacter));
		SetLifeSpan(0.0f);
		InOwnerCharacter->OnDeathStateChanged.RemoveDynamic(this, &ThisClass::HandleOwnerDeathStateChanged);
		InOwnerCharacter->OnDeathStateChanged.AddDynamic(this, &ThisClass::HandleOwnerDeathStateChanged);

		if (InOwnerCharacter->GetIsDead())
		{
			HandleOwnerDeathStateChanged();
		}
	}
	else
	{
		SetOwner(nullptr);
		SetInstigator(nullptr);
	}
}

bool ANexusWeaponBase::HasOffHandWeapon() const
{
	return OffHandWeaponMesh && OffHandWeaponMesh->GetStaticMesh() != nullptr;
}

void ANexusWeaponBase::Destroyed()
{
	if (OwnerCharacter)
	{
		OwnerCharacter->OnDeathStateChanged.RemoveDynamic(this, &ThisClass::HandleOwnerDeathStateChanged);
	}

	Super::Destroyed();
}

void ANexusWeaponBase::HandleOwnerDeathStateChanged()
{
	if (!HasAuthority() || !OwnerCharacter || !OwnerCharacter->GetIsDead())
	{
		return;
	}

	if (OwnerDeathLifeSpan > 0.0f)
	{
		SetLifeSpan(OwnerDeathLifeSpan);
	}
}
