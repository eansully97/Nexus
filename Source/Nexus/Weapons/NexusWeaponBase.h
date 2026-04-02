// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NexusWeaponBase.generated.h"

class ANexusPlayerCharacter;
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
	FName WeaponSocketName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons|Optional")
	FName OffHandSocketName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons")
	TSubclassOf<UAnimInstance> AnimInstanceClass;

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
	USceneComponent* SceneComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* WeaponMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Optional")
	UStaticMeshComponent* OffHandWeaponMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character")
	ANexusCharacterBase* OwnerCharacter;

public:
	UStaticMeshComponent* GetWeaponMesh() const { return WeaponMesh; }
	bool HasOffHandWeapon() const { return OffHandWeaponMesh->GetStaticMesh() != nullptr; }
	UStaticMeshComponent* GetOffHandWeaponMesh() const { return OffHandWeaponMesh; }
};
