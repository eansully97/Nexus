#pragma once

#include "CoreMinimal.h"
#include "Nexus/Weapons/NexusWeaponBase.h"
#include "NexusMeleeWeaponBase.generated.h"

class ANexusCharacterBase;
class UStaticMeshComponent;

UENUM(BlueprintType)
enum class EWeaponOrientation : uint8
{
	MainHand,
	OffHand,
	Both
};

UCLASS()
class NEXUS_API ANexusMeleeWeaponBase : public ANexusWeaponBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void StartHitscan();

	UFUNCTION(BlueprintCallable)
	void EndHitscan();

protected:
	virtual void BeginPlay() override;

	UFUNCTION(Server, Reliable)
	void ServerStartHitscan();

	UFUNCTION(Server, Reliable)
	void ServerEndHitscan();

	void StartHitscan_Internal();
	void EndHitscan_Internal();
	void Hitscan();
	void DoHitscan();

	void ProcessWeaponTrace(
		const UStaticMeshComponent* MeshToTrace,
		TArray<TObjectPtr<ANexusCharacterBase>>& AlreadyHitCharacters) const;

	bool RefreshOwnerCharacter();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapons")
	EWeaponOrientation WeaponOrientation = EWeaponOrientation::MainHand;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapons")
	float DamageHitScanRadius = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapons")
	float DamageToDeal = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapons")
	float HitscanInterval = 0.03f;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ANexusCharacterBase>> AlreadyHitCharactersInWindow;

	FTimerHandle HitscanTimerHandle;
};