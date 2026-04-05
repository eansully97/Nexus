#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NexusWeaponsManager.generated.h"

class ANexusPlayerCharacter;
class ANexusWeaponBase;
class UAnimInstance;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class NEXUS_API UNexusWeaponsManager : public UActorComponent
{
	GENERATED_BODY()

public:
	UNexusWeaponsManager();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_EquippedWeapon, Category="Weapons")
	TObjectPtr<ANexusWeaponBase> EquippedWeapon = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapons")
	TObjectPtr<ANexusPlayerCharacter> OwnerCharacter = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapons")
	TSubclassOf<UAnimInstance> DefaultAnimInstanceClass;

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_EquippedWeapon();

public:
	UFUNCTION(BlueprintCallable, Category="Weapons")
	void Equip(TSubclassOf<ANexusWeaponBase> WeaponClassToEquip);

	UFUNCTION(BlueprintCallable, Category="Weapons")
	void EquipOrSwap(TSubclassOf<ANexusWeaponBase> WeaponClassToEquip);

	UFUNCTION(BlueprintCallable, Category="Weapons")
	void UnequipCurrentWeapon();

	UFUNCTION(BlueprintPure, Category="Weapons")
	bool HasWeaponEquipped() const { return EquippedWeapon != nullptr; }

	void ApplyEquippedWeaponState();
	void AttachEquippedWeapon();
	void SetEquippedWeaponProperties() const;
	void SetUnarmedProperties() const;
};