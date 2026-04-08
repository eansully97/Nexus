#include "Nexus/Components/CharacterClassComponent.h"

#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Materials/MaterialInterface.h"
#include "Nexus/Character/Player/NexusPlayerCharacter.h"
#include "Nexus/Components/NexusWeaponsManager.h"
#include "Nexus/DataAssets/CharacterClassInfo.h"
#include "Nexus/PlayerState/NexusPlayerState.h"

namespace
{
void ApplySkeletalMeshMaterials(
	USkeletalMeshComponent* MeshComp,
	const USkeletalMesh* SourceMesh,
	const TArray<TObjectPtr<UMaterialInterface>>& MaterialOverrides)
{
	if (!MeshComp)
	{
		return;
	}

	const int32 SourceMaterialCount = SourceMesh ? SourceMesh->GetMaterials().Num() : 0;
	const int32 TotalMaterialCount = FMath::Max(SourceMaterialCount, MeshComp->GetNumMaterials());

	for (int32 MaterialIndex = 0; MaterialIndex < TotalMaterialCount; ++MaterialIndex)
	{
		UMaterialInterface* DesiredMaterial = nullptr;

		if (MaterialOverrides.IsValidIndex(MaterialIndex) && MaterialOverrides[MaterialIndex])
		{
			DesiredMaterial = MaterialOverrides[MaterialIndex].Get();
		}
		else if (SourceMesh && SourceMesh->GetMaterials().IsValidIndex(MaterialIndex))
		{
			DesiredMaterial = SourceMesh->GetMaterials()[MaterialIndex].MaterialInterface;
		}

		MeshComp->SetMaterial(MaterialIndex, DesiredMaterial);
	}
}
}

UCharacterClassComponent::UCharacterClassComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UCharacterClassComponent::ApplyClassFromPlayerState(ANexusPlayerState* PS)
{
	if (!PS)
	{
		ResetAppliedClass();
		return;
	}

	ApplyClassInfo(PS->GetCharacterClassInfo());
}

void UCharacterClassComponent::ApplyClassInfo(UCharacterClassInfo* InClassInfo)
{
	if (bClassApplied && AppliedClassInfo == InClassInfo)
	{
		return;
	}

	AppliedClassInfo = InClassInfo;
	bClassApplied = AppliedClassInfo != nullptr;
	ApplyCharacterPresentation();
}

void UCharacterClassComponent::ResetAppliedClass()
{
	AppliedClassInfo = nullptr;
	bClassApplied = false;
	ApplyCharacterPresentation();
}

void UCharacterClassComponent::CacheDefaultCharacterMesh()
{
	if (bCachedDefaultCharacterMesh)
	{
		return;
	}

	if (const ANexusPlayerCharacter* OwnerCharacter = Cast<ANexusPlayerCharacter>(GetOwner()))
	{
		DefaultCharacterMesh = OwnerCharacter->GetDefaultVisualCharacterMesh();

		if (!DefaultCharacterMesh)
		{
			if (const USkeletalMeshComponent* MeshComp = OwnerCharacter->GetMesh())
			{
				DefaultCharacterMesh = MeshComp->GetSkeletalMeshAsset();
			}
		}

		if (const USkeletalMeshComponent* MeshComp = OwnerCharacter->GetMesh())
		{
			DefaultCharacterMaterials.Reset();

			const int32 MaterialCount = FMath::Max(
				DefaultCharacterMesh ? DefaultCharacterMesh->GetMaterials().Num() : 0,
				MeshComp->GetNumMaterials());
			DefaultCharacterMaterials.Reserve(MaterialCount);

			for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex)
			{
				DefaultCharacterMaterials.Add(MeshComp->GetMaterial(MaterialIndex));
			}
		}
	}

	bCachedDefaultCharacterMesh = true;
}

void UCharacterClassComponent::ApplyCharacterPresentation()
{
	ANexusPlayerCharacter* OwnerCharacter = Cast<ANexusPlayerCharacter>(GetOwner());
	if (!OwnerCharacter)
	{
		return;
	}

	USkeletalMeshComponent* MeshComp = OwnerCharacter->GetMesh();
	if (!MeshComp)
	{
		return;
	}

	CacheDefaultCharacterMesh();

	USkeletalMesh* DesiredMesh = AppliedClassInfo
		? AppliedClassInfo->GetResolvedCharacterMesh()
		: DefaultCharacterMesh.Get();

	if (MeshComp->GetSkeletalMeshAsset() != DesiredMesh)
	{
		MeshComp->SetSkeletalMeshAsset(DesiredMesh);
	}

	if (AppliedClassInfo)
	{
		ApplySkeletalMeshMaterials(
			MeshComp,
			DesiredMesh,
			AppliedClassInfo->CharacterData.CharacterMaterialOverrides);
	}
	else
	{
		ApplySkeletalMeshMaterials(
			MeshComp,
			DefaultCharacterMesh.Get(),
			DefaultCharacterMaterials);
	}

	if (UNexusWeaponsManager* WeaponsManager = OwnerCharacter->GetWeaponsManager())
	{
		if (WeaponsManager->HasWeaponEquipped())
		{
			WeaponsManager->ApplyEquippedWeaponState();
		}
		else
		{
			WeaponsManager->SetUnarmedProperties();
		}
	}
}
