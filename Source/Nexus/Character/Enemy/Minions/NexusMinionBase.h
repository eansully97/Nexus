#pragma once

#include "CoreMinimal.h"
#include "Nexus/NexusAbilityGrant.h"
#include "Nexus/Character/Enemy/NexusEnemyBase.h"
#include "NexusMinionBase.generated.h"

class ANexusCapturePoint;
class ANexusCharacterBase;
class UNexusGameplayAbility;
class UAnimInstance;
class USkeletalMesh;

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

	UFUNCTION(BlueprintCallable, Category="Combat")
	bool RotateTowardsCurrentTargetForHitscan(float DeltaTime);

	UFUNCTION(BlueprintPure, Category="Combat")
	bool IsFacingActorForHitscan(const AActor* ActorToFace) const;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ANexusCharacterBase>> AlreadyHitCharactersInWindow;

	FTimerHandle HitscanTimerHandle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat")
	float DamageHitScanRadius = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat")
	float DamageToDeal = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat")
	float HitscanInterval = 0.03f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat|Facing")
	float AttackTurnRateDegreesPerSecond = 720.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat|Facing", meta=(ClampMin="0.0", ClampMax="1.0"))
	float AttackFacingDotThreshold = 0.75f;

	UFUNCTION(BlueprintCallable)
	void InitializeMinion(ANexusCapturePoint* InTargetCapturePoint, ENexusTeamID InTeamID);

	UFUNCTION(BlueprintCallable)
	void HandleReachedCapturePoint(ANexusCapturePoint* CapturePoint);

protected:
	virtual void BeginPlay() override;
	virtual void ApplyTeamVisuals() const override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void InitializeCombatLoadout() override;
	virtual TArray<FNexusAbilityGrant> GetClassAbilitiesToGrant() const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="AbilitySystem")
	TArray<FNexusAbilityGrant> AdditionalMinionAbilityGrants;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="AbilitySystem")
	TSubclassOf<UNexusGameplayAbility> DefaultAttackAbilityClass = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="AbilitySystem")
	int32 DefaultAttackAbilityLevel = 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AbilitySystem")
	bool bMinionLoadoutInitialized = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Objective")
	ANexusCapturePoint* TargetCapturePoint = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AI")
	float AggroRange = 500.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AI")
	float DefendLeashRadius = 685.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AI")
	float CapturePointPriorityBonus = 10000.f;

	UPROPERTY(ReplicatedUsing=OnRep_CurrentTarget, VisibleAnywhere, BlueprintReadOnly, Category="AI")
	TObjectPtr<AActor> CurrentTarget = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visual")
	TObjectPtr<USkeletalMesh> TeamAMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visual")
	TObjectPtr<USkeletalMesh> TeamBMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visual")
	TSubclassOf<UAnimInstance> MinionAnimClass = nullptr;

	FTimerHandle TargetScanTimerHandle;
	bool bAtCapturePoint = false;

	void EnsureDefaultVisualSetup();
	USkeletalMesh* GetDesiredTeamMesh() const;

	void StartTargetScan();
	void StopTargetScan();
	void UpdateTargetActor();

	UFUNCTION()
	void OnRep_CurrentTarget();

	AActor* FindBestTarget() const;
	float ScoreTarget(ANexusCharacterBase* Candidate) const;
	bool IsValidTarget(ANexusCharacterBase* Candidate) const;
	bool IsWithinDefendLeash(const AActor* Actor) const;
	bool IsInsideMyCaptureContext(const ANexusCharacterBase* Candidate) const;
};
