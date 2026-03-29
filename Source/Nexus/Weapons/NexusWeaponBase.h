// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NexusWeaponBase.generated.h"

class UNexusGameplayAbility;
class ANexusWeaponBase;
class ANexusCharacterBase;

USTRUCT(BlueprintType)
struct FWeaponConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons")
	TArray<TSubclassOf<UNexusGameplayAbility>> AbilitiesToGrant;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons")
	FName SocketName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons")
	TSubclassOf<UAnimInstance> AnimInstanceClass;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons")
	FVector SocketOffset;
};

UCLASS()
class NEXUS_API ANexusWeaponBase : public AActor
{
	GENERATED_BODY()

public:
	ANexusWeaponBase();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons")
	FWeaponConfig WeaponConfig;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* WeaponMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character")
	ANexusCharacterBase* OwnerCharacter;

public:
	UStaticMeshComponent* GetWeaponMesh() const { return WeaponMesh; }
};
