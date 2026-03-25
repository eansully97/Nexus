// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "Nexus/NexusTeamTypes.h"
#include "NexusCharacterBase.generated.h"

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
	virtual void OnRep_PlayerState() override;
	void SyncTeamFromPlayerState();
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	UFUNCTION()
	virtual void OnRep_TeamID();

	UFUNCTION()
	void OnRep_IsDead();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AbilitySystem")
	EGameplayEffectReplicationMode ReplicationMode{EGameplayEffectReplicationMode::Mixed};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AbilitySystem")
	TArray<TSubclassOf<UGameplayAbility>> StartingAbilities;

	float CachedWalkSpeed;
	float CachedAcceleration;
	float CachedJumpHeight;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation Flag")
	bool bUseUpperBody{false};

public:	
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	TArray<FGameplayAbilitySpecHandle> GrantedAbilitySpecHandles;

	UFUNCTION(BlueprintCallable, Category = "AbilitySystem")
	TArray<FGameplayAbilitySpecHandle> GrantAbilities(TArray<TSubclassOf<UGameplayAbility>> AbilitiesToGrant);

	UFUNCTION(BlueprintCallable, Category = "AbilitySystem")
	void RemoveAbilities(TArray<FGameplayAbilitySpecHandle> SpecHandles);

	UFUNCTION(BlueprintCallable, Category = "AbilitySystem")
	void SendAbilitiesChangedEvent();

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
};
