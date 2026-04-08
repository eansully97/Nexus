#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Nexus/GameplayAbilitySystem/Abilities/NexusMontageTriggeredAbility.h"
#include "GA_Repulse.generated.h"

UCLASS()
class NEXUS_API UGA_Repulse : public UNexusMontageTriggeredAbility
{
	GENERATED_BODY()

public:
	UGA_Repulse();

protected:
	virtual bool ExecuteTriggeredAction() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Blast")
	FName MuzzleSocketName = TEXT("Muzzle");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Blast")
	float TraceDistance = 15000.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Blast")
	float BlastRadius = 450.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Blast")
	float BlastForwardOffset = 100.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Blast")
	bool bFlattenBlastDirectionToPlane = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Blast", meta=(ClampMin="-1.0", ClampMax="1.0"))
	float MinForwardDot = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Blast")
	float KnockbackStrength = 1400.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Blast")
	float Damage = 15.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Blast")
	float KnockbackUpwardStrength = 75.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Blast")
	bool bScaleKnockbackByDistance = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Blast", meta=(ClampMin="0.0"))
	float MinimumDistanceStrengthMultiplier = 0.7f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GameplayCue")
	FGameplayTag RepulseMuzzleShockwaveCueTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Debug")
	bool bDrawDebugRepulse = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Debug")
	float DebugDrawTime = 1.5f;

	UFUNCTION(BlueprintCallable, Category="Blast")
	bool ExecuteRepulseBlast();

	bool GetRepulseOriginData(
		FVector& OutOrigin,
		FVector& OutForwardDirection) const;

	bool GetAimHitLocation(FVector& OutHitLocation) const;

	void DrawRepulseDebug(
		const FVector& Origin,
		const FVector& ForwardDirection) const;
};