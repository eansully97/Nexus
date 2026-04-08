#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CharacterClassComponent.generated.h"

class UCharacterClassInfo;
class ANexusPlayerState;
class UMaterialInterface;
class USkeletalMesh;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class NEXUS_API UCharacterClassComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCharacterClassComponent();

	void ApplyClassFromPlayerState(ANexusPlayerState* PS);
	void ApplyClassInfo(UCharacterClassInfo* InClassInfo);
	void ResetAppliedClass();

	UFUNCTION(BlueprintPure, Category="Class")
	UCharacterClassInfo* GetAppliedClassInfo() const { return AppliedClassInfo; }

	UFUNCTION(BlueprintPure, Category="Class")
	bool HasAppliedClass() const { return bClassApplied && AppliedClassInfo != nullptr; }

protected:
	UPROPERTY()
	TObjectPtr<UCharacterClassInfo> AppliedClassInfo = nullptr;

	UPROPERTY()
	bool bClassApplied = false;

	UPROPERTY(Transient)
	TObjectPtr<USkeletalMesh> DefaultCharacterMesh = nullptr;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInterface>> DefaultCharacterMaterials;

	bool bCachedDefaultCharacterMesh = false;

	void CacheDefaultCharacterMesh();
	void ApplyCharacterPresentation();
};
