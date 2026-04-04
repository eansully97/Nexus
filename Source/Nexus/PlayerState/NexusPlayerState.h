#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "GameplayTagContainer.h"
#include "Nexus/NexusAbilityGrant.h"
#include "Nexus/NexusEnumTypes.h"
#include "Nexus/DataAssets/CharacterClassInfo.h"
#include "NexusPlayerState.generated.h"

class ANexusCharacterBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerProfileChanged);

USTRUCT(BlueprintType)
struct FNexusPersistentCooldown
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag CooldownTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ExpireWorldTimeSeconds = 0.f;
};

UCLASS()
class NEXUS_API ANexusPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	ANexusPlayerState();

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(ReplicatedUsing=OnRep_TeamID, EditAnywhere, BlueprintReadOnly, Category="Profile", meta=(AllowPrivateAccess="true"))
	ENexusTeamID TeamID = ENexusTeamID::Neutral;

	UPROPERTY(ReplicatedUsing=OnRep_CharacterClassInfo, EditAnywhere, BlueprintReadOnly, Category="Profile", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UCharacterClassInfo> CharacterClassInfo = nullptr;

	UPROPERTY(ReplicatedUsing=OnRep_SelectionLockedIn, BlueprintReadOnly, Category="Profile", meta=(AllowPrivateAccess="true"))
	bool bSelectionLockedIn = false;

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

	float GetRemainingPersistentCooldownTime(const FNexusPersistentCooldown& PersistentCooldown) const;

	bool TryApplySinglePersistentCooldownToCharacter(
		ANexusCharacterBase* Character,
		const FNexusPersistentCooldown& PersistentCooldown) const;

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

	void CapturePersistentCombatStateFromCharacter(ANexusCharacterBase* Character);
	void ApplyPersistentCombatProfileToCharacter(ANexusCharacterBase* Character);
	void TryApplyPersistentCooldownsToCharacter(ANexusCharacterBase* Character);

	TArray<FNexusAbilityGrant> GetWeaponAbilityGrants() const;
	TArray<FNexusAbilityGrant> GetClassAbilityGrants() const;

public:
	UPROPERTY(BlueprintAssignable, Category="Events")
	FOnPlayerProfileChanged OnPlayerProfileChanged;
};