#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "Nexus/NexusAbilityGrant.h"
#include "Nexus/NexusEnumTypes.h"
#include "NexusCharacterBase.generated.h"

class ANexusCapturePoint;
class ANexusPlayerState;
class UAbilitySystemComponent;
class UAnimMontage;
class UBasicAttributeSet;
class UUserWidget;
class UWidgetComponent;
struct FOnAttributeChangeData;
class UGameplayEffect;
class UNexusAbilitySystemComponent;
class UNexusEffectSet;
class UNexusGameplayAbility;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCombatStateChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTeamChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDeathStateChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCapturePointChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGrantedAbilitiesChanged);

USTRUCT(BlueprintType)
struct FNexusSetByCallerMagnitude
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Effects")
	FGameplayTag DataTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Effects")
	float Magnitude = 0.0f;
};

USTRUCT(BlueprintType)
struct FNexusEffectApplicationParams
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Effects")
	TSubclassOf<UGameplayEffect> EffectClass = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Effects")
	float EffectLevel = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Effects")
	TArray<FNexusSetByCallerMagnitude> SetByCallerMagnitudes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Effects|Context")
	TObjectPtr<AActor> InstigatorActor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Effects|Context")
	TObjectPtr<AActor> EffectCauserActor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Effects|Context")
	TObjectPtr<UObject> SourceObject = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Effects|Context")
	bool bIncludeHitResult = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Effects|Context")
	FHitResult HitResult;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Effects|Context")
	bool bUseHitResultImpactPointAsOrigin = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Effects|Rules")
	bool bSkipIfTargetIsDead = true;
};

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
	TObjectPtr<UNexusEffectSet> EffectSet = nullptr;

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

	UPROPERTY(BlueprintAssignable, Category="Events")
	FOnCombatStateChanged OnCombatStateChanged;

	UPROPERTY(BlueprintAssignable, Category="Events")
	FOnTeamChanged OnTeamChanged;

	UPROPERTY(BlueprintAssignable, Category="Events")
	FOnDeathStateChanged OnDeathStateChanged;

	UPROPERTY(BlueprintAssignable, Category="Events")
	FOnCapturePointChanged OnCapturePointChanged;

	UPROPERTY(BlueprintAssignable, Category="Events")
	FOnGrantedAbilitiesChanged OnGrantedAbilitiesChanged;

	UFUNCTION(BlueprintCallable, Category="Combat")
	TArray<ANexusCharacterBase*> PerformSphereTraceForValidEnemies(
		const FVector& StartLocation,
		const FVector& EndLocation,
		float Radius,
		TArray<FHitResult>& OutValidHitResults);

	UFUNCTION(BlueprintCallable, Category="Combat")
	ENexusHitResolutionResult ResolveMeleeHit(
		ANexusCharacterBase* Target,
		const FHitResult& HitResult,
		float Damage);

	void DebugLogGrantedAbilities(const FString& Context) const;

public:
	UFUNCTION(BlueprintCallable, Category="Effects")
	void ApplyEffectToTarget(
		ANexusCharacterBase* Target,
		const FNexusEffectApplicationParams& Params);

	UFUNCTION(BlueprintCallable, Category="AbilitySystem")
	bool SendGameplayEventToSelf(
		const FGameplayTag& EventTag,
		AActor* TargetActor = nullptr,
		AActor* OptionalObject = nullptr);

	UFUNCTION(BlueprintCallable, Category="Effects")
	void ApplyEffectToTargets(
		const TArray<ANexusCharacterBase*>& Targets,
		const FNexusEffectApplicationParams& Params);

	UFUNCTION(BlueprintCallable, Category="Effects")
	void ApplyDamageToTargetWithDuration(
		ANexusCharacterBase* Target,
		float Damage,
		float Duration);

	UFUNCTION(BlueprintCallable, Category="Effects")
	void ApplyHealToTarget(
		ANexusCharacterBase* Target,
		float HealAmount);

	UFUNCTION(BlueprintCallable, Category="Effects")
	void ApplyHealToTargetWithDuration(
		ANexusCharacterBase* Target,
		float HealAmount,
		float Duration);


	FGameplayEffectSpecHandle BuildEffectSpec(
		const FNexusEffectApplicationParams& Params) const;

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

	void ApplyDamageToTargetWithCueParams(
		ANexusCharacterBase* Target,
		float Damage,
		AActor* InstigatorActor,
		AActor* EffectCauserActor,
		const FHitResult* HitResult,
		UObject* SourceObject = nullptr);

	UFUNCTION(BlueprintPure, Category="State")
	bool GetIsDead() const { return bIsDead; }

	UFUNCTION(BlueprintPure, Category="AbilitySystem")
	UBasicAttributeSet* GetBasicAttributeSet() const { return BasicAttributeSet; }

	UFUNCTION(BlueprintPure, Category="UI")
	UWidgetComponent* GetOverheadWidgetComponent() const { return OverheadWidgetComponent; }

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
	TObjectPtr<UNexusAbilitySystemComponent> AbilitySystemComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AbilitySystem")
	TObjectPtr<UBasicAttributeSet> BasicAttributeSet = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="AbilitySystem")
	EGameplayEffectReplicationMode AbilityReplicationMode = EGameplayEffectReplicationMode::Mixed;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Animation")
	TObjectPtr<UAnimMontage> DeathMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Animation")
	TObjectPtr<UAnimMontage> StunMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="State|Death")
	float DeathLifeSpan = 3.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Combat|Deflect")
	float DeflectStunDuration = 1.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Combat|Deflect")
	float DeflectMinDotThreshold = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Combat|Deflect")
	bool bPlayDeflectCueOnDefender = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Combat|Deflect")
	bool bPlayDeflectCueOnAttacker = false;

	UPROPERTY(Transient, BlueprintReadOnly, Category="UI")
	TObjectPtr<UWidgetComponent> OverheadWidgetComponent = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="UI")
	TSubclassOf<UUserWidget> OverheadWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="UI")
	FVector OverheadWidgetRelativeLocation = FVector(0.0f, 0.0f, 120.0f);

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
	virtual void HandleMovementAttributeChanged(const FOnAttributeChangeData& ChangeData);

	void RefreshDeadGameplayTag();

	virtual void InitializeAbilityActorInfo();
	virtual void InitializeFromPlayerState();
	virtual void InitializeCombatLoadout();
	virtual void BindMovementAttributeDelegates();
	virtual void ApplyMovementAttributesToMovementComponent() const;
	virtual void InitializeOverheadWidget();
	virtual void RefreshOverheadWidget();

	virtual TArray<FNexusAbilityGrant> GetClassAbilitiesToGrant() const;

	virtual void DisableCharacterForDeath();

	void HandleSuccessfulDeflect(
		ANexusCharacterBase* Attacker,
		const FHitResult& HitResult);
	
public:
	void RebuildCombatLoadout();

	TArray<FGameplayAbilitySpecHandle>& GetHandleArrayForSource(ENexusAbilitySource Source);
	bool HasGrantedAbilityClass(TSubclassOf<UNexusGameplayAbility> AbilityClass) const;
	bool HasGrantedAbilityTag(const FGameplayTag& AbilityTag) const;

	void BroadcastGrantedAbilitiesChanged();
	void BroadcastLegacyCombatStateChanged();
};
