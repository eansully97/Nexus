// Fill out your copyright notice in the Description page of Project Settings.


#include "NexusWeaponBase.h"

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
