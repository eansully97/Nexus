#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "Nexus/NexusAbilityGrant.h"
#include "Nexus/NexusEnumTypes.h"
#include "NexusCharacterBase.generated.h"

class UNexusEffectSet;
class UNexusAbilitySystemComponent;
class UAbilitySystemComponent;
class UBasicAttributeSet;
class UNexusGameplayAbility;
class UGameplayEffect;
class UAnimMontage;
class ANexusCapturePoint;
class ANexusPlayerState;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCombatStateChanged);

UENUM(BlueprintType)
enum class ENexusHitResolutionResult : uint8
{
	None UMETA(DisplayName="None"),
	Damaged UMETA(DisplayName="Damaged"),
	Deflected UMETA(DisplayName="Deflected"),
	Parried UMETA(DisplayName="Parried")
};

UENUM(BlueprintType)
enum class ENexusAbilitySource : uint8
{
	Class,
	Weapon
};

UCLASS()
class NEXUS_API ANexusCharacterBase : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ANexusCharacterBase();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Effects")
	TObjectPtr<UNexusEffectSet> EffectSet;

protected:
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_Controller() override;
	virtual void OnRep_PlayerState() override;
	virtual void PostInitializeComponents() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(ReplicatedUsing=OnRep_TeamID, VisibleAnywhere, BlueprintReadOnly, Category="Team")
	ENexusTeamID TeamID = ENexusTeamID::Neutral;

	UPROPERTY(ReplicatedUsing=OnRep_IsDead, VisibleAnywhere, BlueprintReadOnly, Category="State")
	bool bIsDead = false;

	UPROPERTY(ReplicatedUsing=OnRep_CurrentCapturePoint, VisibleAnywhere, BlueprintReadOnly, Category="Objective")
	TObjectPtr<ANexusCapturePoint> CurrentCapturePoint = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AbilitySystem")
	TObjectPtr<UNexusAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AbilitySystem")
	TObjectPtr<UBasicAttributeSet> BasicAttributeSet;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="AbilitySystem")
	EGameplayEffectReplicationMode AbilityReplicationMode = EGameplayEffectReplicationMode::Mixed;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Animation")
	TObjectPtr<UAnimMontage> DeathMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Animation")
	TObjectPtr<UAnimMontage> StunMontage = nullptr;

	TArray<FGameplayAbilitySpecHandle> ClassAbilityHandles;
	TArray<FGameplayAbilitySpecHandle> WeaponAbilityHandles;

	UFUNCTION()
	virtual void OnRep_TeamID();

	UFUNCTION()
	virtual void OnRep_IsDead();

	UFUNCTION()
	virtual void OnRep_CurrentCapturePoint();

	virtual void HandleTeamChanged();
	virtual void HandleDeathStateChanged();
	virtual void HandleCurrentCapturePointChanged();

	void RefreshDeadGameplayTag();

	virtual void InitializeAbilityActorInfo();
	virtual void InitializeFromPlayerState();
	virtual void InitializeCombatLoadout();

	virtual TArray<FNexusAbilityGrant> GetClassAbilitiesToGrant() const;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Combat|Deflect")
	float DeflectStunDuration = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Combat|Deflect")
	float DeflectMinDotThreshold = 0.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Combat|Deflect")
	bool bPlayDeflectCueOnDefender = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Combat|Deflect")
	bool bPlayDeflectCueOnAttacker = false;

	void HandleSuccessfulDeflect(
		ANexusCharacterBase* Attacker,
		const FHitResult& HitResult);

public:
	UFUNCTION(BlueprintCallable, Category="Combat")
	ENexusHitResolutionResult ResolveMeleeHit(
		ANexusCharacterBase* Target,
		const FHitResult& HitResult,
		float Damage);

	void DebugLogGrantedAbilities(const FString& Context) const;

	UFUNCTION(BlueprintCallable, Category="Combat|Deflect")
	bool CanParryMeleeHit(ANexusCharacterBase* Attacker) const;

	UFUNCTION(BlueprintCallable, Category="Combat|Deflect")
	bool IsAttackWithinDeflectAngle(
		ANexusCharacterBase* Attacker,
		float MinDotThreshold = 0.2f) const;

public:
	UFUNCTION(BlueprintCallable, Category="Team")
	void SetTeamID(ENexusTeamID NewTeamID);

	UFUNCTION(BlueprintPure, Category="Team")
	ENexusTeamID GetTeamID() const { return TeamID; }

	UFUNCTION(BlueprintCallable, Category="Team")
	virtual void ApplyTeamVisuals() const;

	UFUNCTION(BlueprintCallable, Category="Team")
	bool IsFriendlyTo(AActor* OtherActor) const;

	UFUNCTION(BlueprintCallable, Category="Team")
	bool IsEnemyTo(AActor* OtherActor) const;

	UFUNCTION(BlueprintCallable, Category="Objective")
	void SetCurrentCapturePoint(ANexusCapturePoint* NewCapturePoint);

	UFUNCTION(BlueprintPure, Category="Objective")
	ANexusCapturePoint* GetCurrentCapturePoint() const { return CurrentCapturePoint; }

	UFUNCTION(BlueprintCallable, Category="State")
	virtual void Die();

	UFUNCTION(BlueprintImplementableEvent, Category="State")
	void BP_OnDeathStarted();

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_DeathAnimation();

	void PlayDeathAnimation();

	virtual void ApplyDeathState_Server();
	virtual void ApplyDeathPresentation();

	UFUNCTION(BlueprintCallable, Category="State")
	void EnterRagdoll();

	UFUNCTION(BlueprintCallable, Category="AbilitySystem")
	TArray<FGameplayAbilitySpecHandle> GrantAbilitySet(
		ENexusAbilitySource Source,
		const TArray<FNexusAbilityGrant>& AbilityGrants,
		UObject* SourceObject = nullptr);

	UFUNCTION(BlueprintCallable, Category="AbilitySystem")
	void ClearAbilitySet(ENexusAbilitySource Source);

	UFUNCTION(BlueprintCallable, Category="AbilitySystem")
	void ClearAllGrantedAbilitySets();

protected:
	TArray<FGameplayAbilitySpecHandle>& GetHandleArrayForSource(ENexusAbilitySource Source);
	bool HasGrantedAbilityClass(TSubclassOf<UNexusGameplayAbility> AbilityClass) const;
	bool HasGrantedAbilityTag(const FGameplayTag& AbilityTag) const;

public:
	UPROPERTY(BlueprintAssignable, Category="Events")
	FOnCombatStateChanged OnCombatStateChanged;

	UFUNCTION(BlueprintCallable, Category="Combat")
	TArray<ANexusCharacterBase*> PerformSphereTraceForValidEnemies(
		const FVector& StartLocation,
		const FVector& EndLocation,
		float Radius,
		TArray<FHitResult>& OutValidHitResults);

	void RebuildCombatLoadout();

	UFUNCTION(BlueprintCallable, Category="Effects")
	void ApplyGameplayEffectSpecsToTarget(
		const TArray<FGameplayEffectSpecHandle>& EffectSpecHandles,
		ANexusCharacterBase* TargetToEffect);

	UFUNCTION(BlueprintCallable, Category="Effects")
	void ApplyGameplayEffectSpecsToTargets(
		const TArray<FGameplayEffectSpecHandle>& EffectSpecHandles,
		const TArray<ANexusCharacterBase*>& Targets);

	UFUNCTION(BlueprintCallable, Category="Effects")
	void ApplySetByCallerEffectToTarget(
		ANexusCharacterBase* Target,
		TSubclassOf<UGameplayEffect> EffectClass,
		float Magnitude,
		FGameplayTag DataTag);

	UFUNCTION(BlueprintCallable, Category="Effects")
	void ApplyStunToTarget(
		ANexusCharacterBase* Target,
		float Duration);

	UFUNCTION(BlueprintCallable, Category="Effects")
	void ApplyDamageToTarget(
		ANexusCharacterBase* Target,
		float Damage);

	UFUNCTION(BlueprintPure, Category="State")
	bool GetIsDead() const { return bIsDead; }
};