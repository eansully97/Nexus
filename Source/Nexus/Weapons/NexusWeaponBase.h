// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NexusWeaponBase.generated.h"

class ANexusCharacterBase;
class UGameplayAbility;

USTRUCT(BlueprintType)
struct FWeaponConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons")
	TArray<TSubclassOf<UGameplayAbility>> AbilitiesToGrant;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons")
	FName SocketName;

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
	FWeaponConfig WeaponData;

protected:

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* WeaponMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character")
	ANexusCharacterBase* OwnerCharacter;

public:

	UFUNCTION(BLueprintCallable)
	void SetOwnerCharacter(AActor* InActor);
};
