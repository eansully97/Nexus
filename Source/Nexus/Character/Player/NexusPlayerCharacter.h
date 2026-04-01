#pragma once

#include "CoreMinimal.h"
#include "Nexus/Character/NexusCharacterBase.h"
#include "NexusPlayerCharacter.generated.h"

class UNexusEnhancedInputComponent;
class UCharacterClassInfo;
class UNexusWeaponsManager;
class UCharacterClassComponent;

UCLASS()
class NEXUS_API ANexusPlayerCharacter : public ANexusCharacterBase
{
	GENERATED_BODY()

public:
	ANexusPlayerCharacter();

	void Input_AbilityPressed(FGameplayTag InputTag);
	void Input_AbilityReleased(FGameplayTag InputTag);
	void Input_Move(const FInputActionValue& Value);
	void Input_Look(const FInputActionValue& Value);
	void Input_JumpPressed(const FInputActionValue& Value);
	void Input_JumpReleased(const FInputActionValue& Value);

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<class UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<class UInputConfigInfo> InputConfig;

protected:
	virtual void InitializeFromPlayerState() override;
	virtual void ApplyTeamVisuals() const override;
	virtual void ApplyDeathState_Server() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	const FGameplayAbilitySpec* FindAbilitySpecByInputTag(FGameplayTag InputTag) const;
	bool TrySendAbilityGameplayEvent(FGameplayTag InputTag);

	UFUNCTION(Server, Reliable)
	void Server_SetPitch(float InPitch);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_SetPitch(float InPitch);

	UPROPERTY(VisibleDefaultsOnly, Replicated, BlueprintReadOnly)
	float PitchOffset;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Respawn")
	float RespawnTime = 6.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UNexusWeaponsManager> WeaponsManager;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UNexusEnhancedInputComponent> NexusEnhancedInputComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UCharacterClassComponent> ClassComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="TeamColor")
	FLinearColor TeamAColor1 = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="TeamColor")
	FLinearColor TeamAColor2 = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="TeamColor")
	FLinearColor TeamBColor1 = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="TeamColor")
	FLinearColor TeamBColor2 = FLinearColor::White;

public:
	UFUNCTION(BlueprintPure)
	UNexusWeaponsManager* GetWeaponsManager() const { return WeaponsManager; }

	UFUNCTION(BlueprintPure)
	UCharacterClassComponent* GetClassComponent() const { return ClassComponent; }

	UFUNCTION()
	UCharacterClassInfo* GetClassInfo();
};