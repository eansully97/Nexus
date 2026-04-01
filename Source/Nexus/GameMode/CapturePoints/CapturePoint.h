// NexusCapturePoint.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Nexus/NexusEnumTypes.h"
#include "CapturePoint.generated.h"

class ANexusCharacterBase;
class USphereComponent;
class ANexusMinionBase;

UCLASS()
class NEXUS_API ANexusCapturePoint : public AActor
{
	GENERATED_BODY()

public:
	ANexusCapturePoint();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable)
	void RegisterCombatUnit(ANexusCharacterBase* Unit);

	UFUNCTION(BlueprintCallable)
	void UnregisterCombatUnit(ANexusCharacterBase* Unit);

	UFUNCTION(BlueprintPure)
	bool IsMinionInside(ANexusMinionBase* Minion) const;

	UFUNCTION(BlueprintCallable)
	ANexusCharacterBase* FindClosestEnemyFor(ANexusCharacterBase* RequestingUnit) const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<USphereComponent> CaptureArea;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UStaticMeshComponent>  CapturePointMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UMaterialInstanceDynamic> MaterialInstance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor TeamAColor;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor TeamBColor;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor UncontestedColor;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor ContestedColor;

	void SetCapturePointMaterial(ENexusTeamID TeamID) const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Capture")
	float RequiredUncontestedTime = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Capture")
	bool bAutoRegisterFromOverlap = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Capture")
	ENexusTeamID CurrentResolvingTeam = ENexusTeamID::Neutral;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Capture")
	bool bIsContested = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Capture")
	float CurrentUncontestedTime = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Capture")
	TArray<TObjectPtr<ANexusMinionBase>> TeamAMinions;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Capture")
	TArray<TObjectPtr<ANexusMinionBase>> TeamBMinions;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Capture")
	TArray<TObjectPtr<ANexusCharacterBase>> TeamAPlayers;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Capture")
	TArray<TObjectPtr<ANexusCharacterBase>> TeamBPlayers;

	UFUNCTION()
	void OnCaptureAreaBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

	UFUNCTION()
	void OnCaptureAreaEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex
	);

	void EvaluateControl(float DeltaTime);
	void ResolveCapture(ENexusTeamID ScoringTeam);
	void CleanupInvalidMinions();

	UFUNCTION(BlueprintImplementableEvent, Category="Abilities")
	void BP_OnResolveCapture();

	int32 GetValidMinionCountForTeam(ENexusTeamID TeamID) const;
	TArray<TObjectPtr<ANexusMinionBase>>& GetMinionArrayForTeam(ENexusTeamID TeamID);
	const TArray<TObjectPtr<ANexusMinionBase>>& GetMinionArrayForTeam(ENexusTeamID TeamID) const;
};