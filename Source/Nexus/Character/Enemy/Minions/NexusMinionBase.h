// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Nexus/GameMode/CapturePoints/CapturePoint.h"
#include "Nexus/Character/Enemy/NexusEnemyBase.h"
#include "NexusMinionBase.generated.h"

UCLASS()
class NEXUS_API ANexusMinionBase : public ANexusEnemyBase
{
	GENERATED_BODY()

public:
	ANexusMinionBase();

	UFUNCTION(BlueprintCallable)
	void InitializeMinion(ANexusCapturePoint* InCapturePoint, ENexusTeamID InTeamID);

	UFUNCTION(BlueprintCallable)
	void HandleReachedCapturePoint(ANexusCapturePoint* CapturePoint);

	void SetAtCapturePoint(bool NewValue);
	void HandleLeftCapturePoint(ANexusCapturePoint* CapturePoint);

protected:
	virtual void BeginPlay() override;
	virtual void OnRep_TeamID() override;
	virtual void ApplyTeamVisuals() const override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Objective")
	TObjectPtr<ANexusCapturePoint> TargetCapturePoint = nullptr;


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AI")
	float AggroRange = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AI")
	float LoseTargetRange = 250.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI")
	AActor* CurrentTarget = nullptr;

	FTimerHandle TargetScanTimerHandle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USkeletalMesh* TeamAMesh{};
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USkeletalMesh* TeamBMesh{};
	

public:
	ANexusCharacterBase* FindTargetInFront();
	bool IsTargetStillValid(AActor* Actor) const;
	void UpdateTargetActor();

};
