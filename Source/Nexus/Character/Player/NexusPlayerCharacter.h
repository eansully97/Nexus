#pragma once

#include "CoreMinimal.h"
#include "InputActionValue.h"
#include "Nexus/Character/NexusCharacterBase.h"
#include "NexusPlayerCharacter.generated.h"

class ANexusPlayerState;
class UCharacterClassInfo;
class UCharacterClassComponent;
class UCombatLoadoutComponent;
class UCameraComponent;
class UGameplayEffect;
class UInputConfigInfo;
class UInputMappingContext;
class UPlayerAttributeSet;
class USkeletalMesh;
class USpringArmComponent;
class UNexusEnhancedInputComponent;
class UNexusGameplayAbility;
class UNexusWeaponsManager;

UCLASS()
class NEXUS_API ANexusPlayerCharacter : public ANexusCharacterBase
{
	GENERATED_BODY()

public:
	ANexusPlayerCharacter();
	virtual void Tick(float DeltaSeconds) override;

	void Input_AbilityPressed(FGameplayTag InputTag);
	void Input_AbilityReleased(FGameplayTag InputTag);
	void Input_Move(const FInputActionValue& Value);
	void Input_Look(const FInputActionValue& Value);
	void Input_JumpPressed(const FInputActionValue& Value);
	void Input_JumpReleased(const FInputActionValue& Value);

	bool ShouldBlockNativeInput() const;

	UFUNCTION(BlueprintPure)
	bool CanAcceptGameplayInput() const;

	UFUNCTION(BlueprintPure)
	UNexusWeaponsManager* GetWeaponsManager() const { return WeaponsManager; }

	UFUNCTION(BlueprintPure)
	UCharacterClassComponent* GetClassComponent() const { return ClassComponent; }

	UFUNCTION(BlueprintPure)
	UCombatLoadoutComponent* GetCombatLoadoutComponent() const { return CombatLoadoutComponent; }

	UFUNCTION(BlueprintPure, Category="AbilitySystem")
	UPlayerAttributeSet* GetPlayerAttributeSet() const { return PlayerAttributeSet; }

	UFUNCTION(BlueprintPure)
	USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	UFUNCTION(BlueprintPure)
	UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	UFUNCTION(BlueprintPure, Category="Visuals")
	USkeletalMesh* GetDefaultVisualCharacterMesh() const { return DefaultVisualCharacterMesh; }

	UFUNCTION()
	UCharacterClassInfo* GetClassInfo();

	UPROPERTY(EditDefaultsOnly, Category="Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, Category="Input")
	TObjectPtr<UInputConfigInfo> InputConfig;

	UPROPERTY(BlueprintReadOnly, Category="Animation")
	float PitchOffset = 0.0f;

protected:
	virtual void InitializeAbilityActorInfo() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_Controller() override;
	virtual void InitializeFromPlayerState() override;
	virtual void InitializeCombatLoadout() override;
	virtual void ApplyDeathState_Server() override;
	virtual void PostInitializeComponents() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	const FGameplayAbilitySpec* FindAbilitySpecByInputTag(FGameplayTag InputTag) const;
	bool TrySendAbilityGameplayEvent(FGameplayTag InputTag);
	bool TryResolveUsableTargetForAbility(const UNexusGameplayAbility* AbilityCDO, ANexusCharacterBase*& OutTargetCharacter) const;
	void BindPlayerAttributeDelegates();
	void ApplyDefaultVisualMesh();
	void RefreshStaminaRegenState();
	void HandleStaminaAttributeChanged(const FOnAttributeChangeData& ChangeData);
	void HandleMaxStaminaAttributeChanged(const FOnAttributeChangeData& ChangeData);

	UFUNCTION(Server, Reliable)
	void Server_SendAbilityTargetedEvent(FGameplayTag InputTag, AActor* TargetActor);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Respawn")
	float RespawnTime = 6.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Effects|Stamina")
	TSubclassOf<UGameplayEffect> StaminaRegenEffect = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Visuals")
	TObjectPtr<USkeletalMesh> DefaultVisualCharacterMesh = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UNexusWeaponsManager> WeaponsManager;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UNexusEnhancedInputComponent> NexusEnhancedInputComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UCharacterClassComponent> ClassComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UCombatLoadoutComponent> CombatLoadoutComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AbilitySystem")
	TObjectPtr<UPlayerAttributeSet> PlayerAttributeSet;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UCameraComponent> FollowCamera;
};
