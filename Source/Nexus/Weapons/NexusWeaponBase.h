#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Nexus/NexusAbilityGrant.h"
#include "NexusWeaponBase.generated.h"

class ANexusPlayerCharacter;
class ANexusCharacterBase;
class UAnimInstance;

USTRUCT(BlueprintType)
struct FWeaponConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapons")
	TArray<FNexusAbilityGrant> AbilityGrants;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapons")
	FName WeaponSocketName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapons|Optional")
	FName OffHandSocketName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapons")
	TSubclassOf<UAnimInstance> AnimInstanceClass;
};

UCLASS()
class NEXUS_API ANexusWeaponBase : public AActor
{
	GENERATED_BODY()

public:
	ANexusWeaponBase();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapons")
	FWeaponConfig WeaponConfig;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USceneComponent> SceneComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> WeaponMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components|Optional")
	TObjectPtr<UStaticMeshComponent> OffHandWeaponMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Character")
	TObjectPtr<ANexusCharacterBase> OwnerCharacter = nullptr;

public:
	UFUNCTION(BlueprintCallable, Category="Weapons")
	void SetOwnerCharacter(ANexusCharacterBase* InOwnerCharacter);

	UFUNCTION(BlueprintPure, Category="Weapons")
	ANexusCharacterBase* GetOwnerCharacter() const { return OwnerCharacter; }

	UFUNCTION(BlueprintPure, Category="Weapons")
	UStaticMeshComponent* GetWeaponMesh() const { return WeaponMesh; }

	UFUNCTION(BlueprintPure, Category="Weapons")
	bool HasOffHandWeapon() const;

	UFUNCTION(BlueprintPure, Category="Weapons")
	UStaticMeshComponent* GetOffHandWeaponMesh() const { return OffHandWeaponMesh; }

protected:
	virtual void Destroyed() override;

	UFUNCTION()
	void HandleOwnerDeathStateChanged();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapons|Cleanup")
	float OwnerDeathLifeSpan = 3.0f;
};
