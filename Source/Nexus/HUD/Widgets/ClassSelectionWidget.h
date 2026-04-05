#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Nexus/NexusAbilityGrant.h"
#include "Nexus/DataAssets/CharacterClassInfo.h"
#include "ClassSelectionWidget.generated.h"

class ANexusPlayerController;
class ANexusWeaponBase;

UCLASS()
class NEXUS_API UClassSelectionWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Selection")
	void SelectClass(UCharacterClassInfo* InClassInfo);

	UFUNCTION(BlueprintCallable, Category="Selection")
	void SelectWeapon(TSubclassOf<ANexusWeaponBase> InWeaponClass);

	UFUNCTION(BlueprintCallable, Category="Selection")
	void SetSelectedClassAbilities(const TArray<FNexusAbilityGrant>& InAbilityGrants);

	UFUNCTION(BlueprintCallable, Category="Selection")
	void ClickReady();

	UFUNCTION(BlueprintCallable, Category="Selection")
	void ClickUnready();

	UFUNCTION(BlueprintPure, Category="Selection")
	UCharacterClassInfo* GetPendingClass() const { return PendingClass; }

	UFUNCTION(BlueprintPure, Category="Selection")
	TSubclassOf<ANexusWeaponBase> GetPendingWeapon() const { return PendingWeapon; }

	UFUNCTION(BlueprintPure, Category="Selection")
	const TArray<FNexusAbilityGrant>& GetPendingClassAbilities() const
	{
		return PendingClassAbilities;
	}

protected:
	UPROPERTY(BlueprintReadOnly, Category="Selection")
	TObjectPtr<UCharacterClassInfo> PendingClass = nullptr;

	UPROPERTY(BlueprintReadOnly, Category="Selection")
	TSubclassOf<ANexusWeaponBase> PendingWeapon = nullptr;

	UPROPERTY(BlueprintReadOnly, Category="Selection")
	TArray<FNexusAbilityGrant> PendingClassAbilities;

private:
	ANexusPlayerController* GetNexusPC() const;
};