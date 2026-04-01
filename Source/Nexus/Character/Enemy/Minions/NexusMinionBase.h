#pragma once

#include "CoreMinimal.h"
#include "Nexus/Character/Enemy/NexusEnemyBase.h"
#include "NexusMinionBase.generated.h"

class ANexusCapturePoint;
class UNexusGameplayAbility;

UCLASS()
class NEXUS_API ANexusMinionBase : public ANexusEnemyBase
{
	GENERATED_BODY()

public:
	ANexusMinionBase();

	UFUNCTION(BlueprintCallable)
	void StartHitscan();

	UFUNCTION(BlueprintCallable)
	void EndHitscan();

	void Hitscan();
	void DoHitscan();

	UPROPERTY(Transient)
	TArray<TObjectPtr<ANexusCharacterBase>> AlreadyHitCharactersInWindow;

	FTimerHandle HitscanTimerHandle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat")
	float DamageHitScanRadius = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat")
	float DamageToDeal = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat")
	float HitscanInterval = 0.03f;

	UFUNCTION(BlueprintCallable)
	void InitializeMinion(ANexusCapturePoint* InTargetCapturePoint, ENexusTeamID InTeamID);

	UFUNCTION(BlueprintCallable)
	void HandleReachedCapturePoint(ANexusCapturePoint* CapturePoint);

protected:
	virtual void BeginPlay() override;
	virtual void InitializeCombatLoadout() override;
	virtual void ApplyTeamVisuals() const override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Objective")
	ANexusCapturePoint* TargetCapturePoint = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AI")
	float AggroRange = 500.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AI")
	float DefendLeashRadius = 685.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AI")
	float CapturePointPriorityBonus = 10000.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI")
	TObjectPtr<AActor> CurrentTarget = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visual")
	TObjectPtr<USkeletalMesh> TeamAMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visual")
	TObjectPtr<USkeletalMesh> TeamBMesh = nullptr;

	FTimerHandle TargetScanTimerHandle;
	bool bAtCapturePoint = false;

	void StartTargetScan();
	void StopTargetScan();
	void UpdateTargetActor();

	AActor* FindBestTarget() const;
	float ScoreTarget(ANexusCharacterBase* Candidate) const;
	bool IsValidTarget(ANexusCharacterBase* Candidate) const;
	bool IsWithinDefendLeash(const AActor* Actor) const;
	bool IsInsideMyCaptureContext(const ANexusCharacterBase* Candidate) const;
};