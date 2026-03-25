// Fill out your copyright notice in the Description page of Project Settings.


#include "NexusWeaponBase.h"

#include "Nexus/Character/NexusCharacterBase.h"


ANexusWeaponBase::ANexusWeaponBase()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	AActor::SetReplicateMovement(true);

	WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetRootComponent(WeaponMesh);
}


void ANexusWeaponBase::BeginPlay()
{
	Super::BeginPlay();
}

void ANexusWeaponBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ANexusWeaponBase::SetOwnerCharacter(AActor* InActor)
{
	OwnerCharacter = Cast<ANexusCharacterBase>(InActor);
	SetOwner(InActor);
}

