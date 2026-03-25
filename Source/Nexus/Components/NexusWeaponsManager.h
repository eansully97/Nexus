// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySpecHandle.h"
#include "Components/ActorComponent.h"
#include "NexusWeaponsManager.generated.h"


class ANexusCharacterBase;
class ANexusWeaponBase;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class NEXUS_API UNexusWeaponsManager : public UActorComponent
{
	GENERATED_BODY()

public:

	UNexusWeaponsManager();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_EquippedWeapon, Category = "Weapons")
	TObjectPtr<ANexusWeaponBase> EquippedWeapon;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapons")
	TObjectPtr<ANexusCharacterBase> OwnerCharacter;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons")
	TSubclassOf<UAnimInstance> DefaultAnimInstanceClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapons")
	TArray<FGameplayAbilitySpecHandle> WeaponAbilities;

protected:

	virtual void BeginPlay() override;

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	
	UFUNCTION()
	void OnRep_EquippedWeapon();

public:

	UFUNCTION(BlueprintCallable)
	void Equip(TSubclassOf<ANexusWeaponBase> WeaponClassToEquip);
	
	void Unequip();
	UFUNCTION(Server,Reliable, BlueprintCallable)
	void Server_Unequip();

	void SetEquippedWeaponProperties() const;
	void SetUnarmedProperties() const;
};
