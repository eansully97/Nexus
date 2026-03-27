// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "Nexus/NexusEnumTypes.h"
#include "NexusCharacterBase.generated.h"

class ANexusCapturePoint;
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAbilitiesChanged);

class UNexusGameplayAbility;
class UNexusWeaponsManager;
class UBasicAttributeSet;

UCLASS()
class NEXUS_API ANexusCharacterBase : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ANexusCharacterBase();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TeamColor")
	FLinearColor TeamAColor1;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TeamColor")
	FLinearColor TeamAColor2;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TeamColor")
	FLinearColor TeamBColor1;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TeamColor")
	FLinearColor TeamBColor2;

	void OnDeathStarted();
	
	UFUNCTION(BlueprintCallable)
	void EnterRagdoll();

	virtual void Die();

	UPROPERTY(ReplicatedUsing = OnRep_TeamID)
	ENexusTeamID TeamID;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Objective")
	TObjectPtr<ANexusCapturePoint> CurrentCapturePoint = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AbilitySystem")
	UAbilitySystemComponent* AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AbilitySystem")
	UBasicAttributeSet* BasicAttributeSet;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UNexusWeaponsManager* WeaponsManager;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Montages")
	UAnimMontage* DeathMontage;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Util", ReplicatedUsing=OnRep_IsDead)
	bool bIsDead = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Util")
	float RespawnTime = 6.f;

protected:

	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_Controller() override;
	virtual void OnRep_PlayerState() override;
	void SyncTeamFromPlayerState();
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	UFUNCTION()
	virtual void OnRep_TeamID();

	UFUNCTION()
	void OnRep_IsDead();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AbilitySystem")
	EGameplayEffectReplicationMode ReplicationMode = EGameplayEffectReplicationMode::Mixed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AbilitySystem")
	TArray<TSubclassOf<UNexusGameplayAbility>> StartingAbilities;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AbilitySystem")
	bool bStartupAbilitiesGranted = false;

	FTimerHandle DeferredAbilitiesChangedHandle;

	UFUNCTION()
	void BroadcastAbilitiesChangedDeferred();

	UFUNCTION(BlueprintImplementableEvent, Category="BlueprintEvents")
	void BP_OnDeathStarted();

	float CachedWalkSpeed;
	float CachedAcceleration;
	float CachedJumpHeight;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation Flag")
	bool bUseUpperBody{false};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Objective")
	bool bAtCapturePoint = false;

public:	
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AbilitySystem")
	TArray<FGameplayAbilitySpecHandle> StartupAbilitySpecHandles;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AbilitySystem")
	TArray<FGameplayAbilitySpecHandle> TemporaryAbilitySpecHandles;

	UFUNCTION(BlueprintCallable, Category = "AbilitySystem")
	TArray<FGameplayAbilitySpecHandle> GrantAbilities(
		const TArray<TSubclassOf<UNexusGameplayAbility>>& AbilitiesToGrant,
		bool bTrackAsStartupAbilities = false,
		bool bTrackAsTemporaryAbilities = false
	);

	UFUNCTION(BlueprintCallable, Category = "AbilitySystem")
	void RemoveAbilities(const TArray<FGameplayAbilitySpecHandle>& SpecHandles);

	UFUNCTION(BlueprintCallable, Category = "AbilitySystem")
	void RemoveStartupAbilities();

	UFUNCTION(BlueprintCallable, Category = "AbilitySystem")
	void RemoveTemporaryAbilities();

	UPROPERTY(BlueprintAssignable, Category="AbilitySystem")
	FOnAbilitiesChanged OnAbilitiesChanged;

	UFUNCTION(Client, Reliable)
	void Client_NotifyAbilitiesChanged();

	UFUNCTION(BlueprintCallable, Category="AbilitySystem")
	void BroadcastAbilitiesChanged();

	UFUNCTION(BlueprintCallable, Category = "Movement")
	void SetCharacterSpeedToZero();

	UFUNCTION(BlueprintCallable, Category = "Movement")
	void RestoreCharacterMovement();

	UFUNCTION(BlueprintCallable)
	ENexusTeamID GetTeamID() const;

	UFUNCTION(BlueprintCallable)
	virtual void ApplyTeamVisuals() const;
	
	UFUNCTION(BlueprintCallable)
	void SetTeamID(ENexusTeamID InTeamID);
	
	UFUNCTION(BlueprintCallable)
	bool IsFriendlyTo(AActor* OtherActor) const;
	
	UFUNCTION(BlueprintCallable)
	bool IsEnemyTo(AActor* OtherActor) const;

	UFUNCTION(BlueprintCallable)
	ANexusCapturePoint* GetCurrentCapturePoint();
};
