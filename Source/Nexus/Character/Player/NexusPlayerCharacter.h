#pragma once

#include "CoreMinimal.h"
#include "Nexus/Character/NexusCharacterBase.h"
#include "NexusPlayerCharacter.generated.h"

class UCharacterClassInfo;
class UNexusWeaponsManager;
class UCharacterClassComponent;

UCLASS()
class NEXUS_API ANexusPlayerCharacter : public ANexusCharacterBase
{
	GENERATED_BODY()

public:
	ANexusPlayerCharacter();

protected:
	virtual void InitializeFromPlayerState() override;
	virtual void ApplyTeamVisuals() const override;
	virtual void ApplyDeathState_Server() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Respawn")
	float RespawnTime = 6.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UNexusWeaponsManager> WeaponsManager;

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