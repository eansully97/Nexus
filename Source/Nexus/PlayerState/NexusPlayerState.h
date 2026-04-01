#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "GameplayTagContainer.h"
#include "Nexus/NexusEnumTypes.h"
#include "Nexus/DataAssets/CharacterClassInfo.h"
#include "NexusPlayerState.generated.h"

class ANexusPlayerCharacter;
class ANexusCharacterBase;
class UNexusGameplayAbility;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerProfileChanged);

USTRUCT(BlueprintType)
struct FNexusPersistentCooldown
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag CooldownTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float RemainingTime = 0.f;
};

UCLASS()
class NEXUS_API ANexusPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	ANexusPlayerState();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<TSubclassOf<UNexusGameplayAbility>> BaseAbilities;

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(ReplicatedUsing=OnRep_TeamID, EditAnywhere, BlueprintReadOnly, Category="Profile", meta=(AllowPrivateAccess="true"))
	ENexusTeamID TeamID = ENexusTeamID::Neutral;

	UPROPERTY(ReplicatedUsing=OnRep_CharacterClassInfo, EditAnywhere, BlueprintReadOnly, Category="Profile", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UCharacterClassInfo> CharacterClassInfo = nullptr;

	UPROPERTY(ReplicatedUsing=OnRep_SelectionLockedIn, BlueprintReadOnly, Category="Profile", meta=(AllowPrivateAccess="true"))
	bool bSelectionLockedIn = false;

	// Persisted across respawn.
	UPROPERTY()
	TArray<FNexusPersistentCooldown> PersistentCooldowns;

	UPROPERTY()
	FGameplayTagContainer PersistentLooseTags;

	UFUNCTION()
	void OnRep_TeamID();

	UFUNCTION()
	void OnRep_CharacterClassInfo();

	UFUNCTION()
	void OnRep_SelectionLockedIn();

	void HandleProfileChanged();

public:
	UFUNCTION(BlueprintCallable, Category="Profile")
	void SetTeamID(ENexusTeamID NewTeamID);

	UFUNCTION(BlueprintPure, Category="Profile")
	ENexusTeamID GetTeamID() const { return TeamID; }

	UFUNCTION(BlueprintCallable, Category="Profile")
	void SetCharacterClassInfo(UCharacterClassInfo* InInfo);

	UFUNCTION(BlueprintPure, Category="Profile")
	UCharacterClassInfo* GetCharacterClassInfo() const { return CharacterClassInfo; }

	UFUNCTION(BlueprintCallable, Category="Profile")
	void SetSelectionLockedIn(bool bLockedIn);

	UFUNCTION(BlueprintPure, Category="Profile")
	bool GetSelectionLockedIn() const { return bSelectionLockedIn; }

	// --- Respawn persistence ---
	void CapturePersistentCombatStateFromCharacter(ANexusCharacterBase* Character);
	void ApplyPersistentCombatProfileToCharacter(ANexusCharacterBase* Character);

	// Integration points for your data assets.
	TArray<TSubclassOf<UNexusGameplayAbility>> GetClassAbilityList() const;
	TArray<TSubclassOf<UNexusGameplayAbility>> GetWeaponAbilityList() const;

public:
	UPROPERTY(BlueprintAssignable, Category="Events")
	FOnPlayerProfileChanged OnPlayerProfileChanged;
};