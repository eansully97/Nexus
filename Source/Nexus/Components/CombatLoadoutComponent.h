#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatLoadoutComponent.generated.h"

class ANexusPlayerCharacter;
class ANexusPlayerState;
class UCharacterClassInfo;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLoadoutApplied);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCombatLoadoutGrantedAbilitiesChanged);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class NEXUS_API UCombatLoadoutComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatLoadoutComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable, Category="Loadout")
	void RefreshFromCurrentPlayerState();

	UFUNCTION(BlueprintCallable, Category="Loadout")
	void ApplyLoadoutFromPlayerState(ANexusPlayerState* PlayerState, bool bApplyPersistentState = true);

	UFUNCTION(BlueprintPure, Category="Loadout")
	UCharacterClassInfo* GetAppliedClassInfo() const { return AppliedClassInfo; }

	UPROPERTY(BlueprintAssignable, Category="Loadout")
	FOnLoadoutApplied OnLoadoutApplied;

	UPROPERTY(BlueprintAssignable, Category="Loadout")
	FOnCombatLoadoutGrantedAbilitiesChanged OnGrantedAbilitiesChanged;

private:
	UPROPERTY()
	TObjectPtr<ANexusPlayerCharacter> OwnerCharacter = nullptr;

	UPROPERTY()
	TObjectPtr<ANexusPlayerState> ObservedPlayerState = nullptr;

	UPROPERTY()
	TObjectPtr<UCharacterClassInfo> AppliedClassInfo = nullptr;

	void BindToObservedPlayerState();
	void UnbindFromObservedPlayerState();

	UFUNCTION()
	void HandleObservedPlayerProfileChanged();

	void BroadcastLoadoutChanged();
};