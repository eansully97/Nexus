#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "Nexus/NexusEnumTypes.h"
#include "NexusCharacterBase.generated.h"

class UNexusEffectSet;
class UNexusAbilitySystemComponent;
class UAbilitySystemComponent;
class UBasicAttributeSet;
class UNexusGameplayAbility;
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
	Base,
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
	
	bool ShouldBlockNativeInput() const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Effects")
	TObjectPtr<UNexusEffectSet> EffectSet;

protected:
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_Controller() override;
	virtual void OnRep_PlayerState() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	

	// --- Replicated shared combat state ---
	UPROPERTY(ReplicatedUsing=OnRep_TeamID, VisibleAnywhere, BlueprintReadOnly, Category="Team")
	ENexusTeamID TeamID = ENexusTeamID::Neutral;

	UPROPERTY(ReplicatedUsing=OnRep_IsDead, VisibleAnywhere, BlueprintReadOnly, Category="State")
	bool bIsDead = false;


	UPROPERTY(ReplicatedUsing=OnRep_CurrentCapturePoint, VisibleAnywhere, BlueprintReadOnly, Category="Objective")
	TObjectPtr<ANexusCapturePoint> CurrentCapturePoint = nullptr;

	// --- GAS ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AbilitySystem")
	TObjectPtr<UNexusAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AbilitySystem")
	TObjectPtr<UBasicAttributeSet> BasicAttributeSet;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="AbilitySystem")
	EGameplayEffectReplicationMode AbilityReplicationMode = EGameplayEffectReplicationMode::Mixed;

	// Optional shared default abilities.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="AbilitySystem")
	TArray<TSubclassOf<UNexusGameplayAbility>> BaseAbilities;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Animation")
	TObjectPtr<UAnimMontage> DeathMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Animation")
	TObjectPtr<UAnimMontage> StunMontage = nullptr;

	// Runtime tracking by source.
	TArray<FGameplayAbilitySpecHandle> BaseAbilityHandles;
	TArray<FGameplayAbilitySpecHandle> ClassAbilityHandles;
	TArray<FGameplayAbilitySpecHandle> WeaponAbilityHandles;

	// --- Rep notifies ---
	UFUNCTION()
	virtual void OnRep_TeamID();

	UFUNCTION()
	virtual void OnRep_IsDead();

	UFUNCTION()
	virtual void OnRep_CurrentCapturePoint();

	// Shared handlers called by both authority and rep-notify.
	virtual void HandleTeamChanged();
	virtual void HandleDeathStateChanged();
	
	virtual void HandleCurrentCapturePointChanged();

	void RefreshDeadGameplayTag();

	// Shared GAS init
	virtual void InitializeAbilityActorInfo();
	virtual void InitializeFromPlayerState();
	virtual void InitializeCombatLoadout();
public:
	// Parry
	UFUNCTION(BlueprintCallable)
	ENexusHitResolutionResult ResolveMeleeHit(
		ANexusCharacterBase* Target,
		const FHitResult& HitResult,
		float Damage
	);
	void DebugLogGrantedAbilities(const FString& Context) const;

	UFUNCTION(BlueprintCallable)
	bool CanParryMeleeHit(
		ANexusCharacterBase* Attacker
	) const;

	UFUNCTION(BlueprintCallable)
	bool IsAttackWithinDeflectAngle(
		ANexusCharacterBase* Attacker,
		float MinDotThreshold = 0.2f
	) const;

	UFUNCTION(BlueprintCallable)
	bool SendParryGameplayEvent(
		ANexusCharacterBase* Attacker,
		const FHitResult& HitResult
	);

public:
	// ---- Team ----
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

	// ---- Capture point occupancy ----
	UFUNCTION(BlueprintCallable, Category="Objective")
	void SetCurrentCapturePoint(ANexusCapturePoint* NewCapturePoint);

	UFUNCTION(BlueprintPure, Category="Objective")
	ANexusCapturePoint* GetCurrentCapturePoint() const { return CurrentCapturePoint; }

	// ---- Death ----
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

	// ---- Abilities by source ----
	UFUNCTION(BlueprintCallable, Category="AbilitySystem")
	TArray<FGameplayAbilitySpecHandle> GrantAbilitySet(
		ENexusAbilitySource Source,
		const TArray<TSubclassOf<UNexusGameplayAbility>>& AbilitiesToGrant);

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

	// Ability Helper Functions
	UFUNCTION(BlueprintCallable)
	TArray<ANexusCharacterBase*> PerformSphereTraceForValidEnemies(
		const FVector& StartLocation,
		const FVector& EndLocation,
		float Radius,
		TArray<FHitResult>& OutValidHitResults);
	
	UFUNCTION(BlueprintCallable)
	void ApplyGameplayEffectSpecsToTarget(
		const TArray<FGameplayEffectSpecHandle>& EffectSpecHandles,
		ANexusCharacterBase* TargetToEffect);

	UFUNCTION(BlueprintCallable)
	void ApplyGameplayEffectSpecsToTargets(
		const TArray<FGameplayEffectSpecHandle>& EffectSpecHandles,
		const TArray<ANexusCharacterBase*>& Targets);

	UFUNCTION(BlueprintCallable)
	void ApplySetByCallerEffectToTarget(ANexusCharacterBase* Target,
		TSubclassOf<UGameplayEffect> EffectClass,
		float Magnitude, FGameplayTag DataTag);

	UFUNCTION(BlueprintCallable)
	void ApplyStunToTarget(ANexusCharacterBase* Target,
		float Duration);

	UFUNCTION(BlueprintCallable)
	void ApplyDamageToTarget(ANexusCharacterBase* Target,
		float Damage);

	UFUNCTION(BlueprintPure)
	bool GetIsDead() const { return bIsDead; }
};