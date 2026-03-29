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
	void InitializeMinion(ANexusCapturePoint* InTargetCapturePoint, ENexusTeamID InTeamID);

	UFUNCTION(BlueprintCallable)
	void HandleReachedCapturePoint(ANexusCapturePoint* CapturePoint);

	UFUNCTION(BlueprintCallable)
	void HandleLeftCapturePoint(ANexusCapturePoint* CapturePoint);

protected:
	virtual void BeginPlay() override;
	virtual void InitializeCombatLoadout() override;
	virtual void ApplyTeamVisuals() const override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Objective")
	ANexusCapturePoint* TargetCapturePoint = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AI")
	float AggroRange = 650.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AI")
	float DefendLeashRadius = 900.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AI")
	float CapturePointPriorityBonus = 10000.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI")
	TObjectPtr<AActor> CurrentTarget = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visual")
	TObjectPtr<USkeletalMesh> TeamAMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visual")
	TObjectPtr<USkeletalMesh> TeamBMesh = nullptr;

	FTimerHandle TargetScanTimerHandle;

	void StartTargetScan();
	void StopTargetScan();
	void UpdateTargetActor();

	AActor* FindBestTarget() const;
	float ScoreTarget(ANexusCharacterBase* Candidate) const;
	bool IsValidTarget(ANexusCharacterBase* Candidate) const;
	bool IsWithinDefendLeash(const AActor* Actor) const;
	bool IsInsideMyCaptureContext(const ANexusCharacterBase* Candidate) const;
};