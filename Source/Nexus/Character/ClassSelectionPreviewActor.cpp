#include "ClassSelectionPreviewActor.h"

#include "Animation/AnimInstance.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Nexus/DataAssets/CharacterClassInfo.h"
#include "Nexus/PlayerState/NexusPlayerState.h"
#include "Nexus/Weapons/NexusWeaponBase.h"
#include "TimerManager.h"

AClassSelectionPreviewActor::AClassSelectionPreviewActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bHideActorWhenNoSelection = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	PreviewCharacterMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("PreviewCharacterMesh"));
	PreviewCharacterMesh->SetupAttachment(Root);
	PreviewCharacterMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PreviewCharacterMesh->SetGenerateOverlapEvents(false);
	PreviewCharacterMesh->SetCastShadow(true);

	PreviewWeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PreviewWeaponMesh"));
	PreviewWeaponMesh->SetupAttachment(PreviewCharacterMesh);
	PreviewWeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PreviewWeaponMesh->SetGenerateOverlapEvents(false);

	PreviewOffHandWeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PreviewOffHandWeaponMesh"));
	PreviewOffHandWeaponMesh->SetupAttachment(PreviewCharacterMesh);
	PreviewOffHandWeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PreviewOffHandWeaponMesh->SetGenerateOverlapEvents(false);
}

void AClassSelectionPreviewActor::BeginPlay()
{
	Super::BeginPlay();

	TryBindToObservedPlayerState();
	RefreshPreview();
}

void AClassSelectionPreviewActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RetryBindTimerHandle);
	}

	UnbindFromObservedPlayerState();

	Super::EndPlay(EndPlayReason);
}

void AClassSelectionPreviewActor::TryBindToObservedPlayerState()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, PreviewOwnerIndex);
	ANexusPlayerState* NewPlayerState = PC ? PC->GetPlayerState<ANexusPlayerState>() : nullptr;

	if (ObservedPlayerState == NewPlayerState && ObservedPlayerState)
	{
		return;
	}

	UnbindFromObservedPlayerState();
	ObservedPlayerState = NewPlayerState;

	if (ObservedPlayerState)
	{
		ObservedPlayerState->OnPlayerProfileChanged.AddDynamic(
			this,
			&ThisClass::HandleObservedPlayerProfileChanged);

		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(RetryBindTimerHandle);
		}
	}
	else
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(
				RetryBindTimerHandle,
				this,
				&ThisClass::TryBindToObservedPlayerState,
				0.25f,
				false);
		}
	}
}

void AClassSelectionPreviewActor::UnbindFromObservedPlayerState()
{
	if (ObservedPlayerState)
	{
		ObservedPlayerState->OnPlayerProfileChanged.RemoveDynamic(
			this,
			&ThisClass::HandleObservedPlayerProfileChanged);
	}

	ObservedPlayerState = nullptr;
}

void AClassSelectionPreviewActor::HandleObservedPlayerProfileChanged()
{
	RefreshPreview();
}

void AClassSelectionPreviewActor::RefreshPreview()
{
	TryBindToObservedPlayerState();

	if (!ObservedPlayerState)
	{
		PreviewCharacterMesh->SetSkeletalMesh(nullptr);
		ApplyPreviewCharacterMaterials(nullptr, {});
		ApplyPreviewAnimClass(nullptr);
		SetPreviewVisible(false);
		ClearWeaponPreview();
		return;
	}

	UCharacterClassInfo* SelectedClass = ObservedPlayerState->GetCharacterClassInfo();
	const TSubclassOf<ANexusWeaponBase> SelectedWeaponClass = ObservedPlayerState->GetSelectedWeaponClass();
	const bool bLockedIn = ObservedPlayerState->GetSelectionLockedIn();

	ApplyClassPreview(SelectedClass, bLockedIn);
	ApplyWeaponPreview(SelectedWeaponClass);
}

void AClassSelectionPreviewActor::ApplyDefaultPreview(bool bLockedIn)
{
	SetPreviewVisible(true);

	if (PreviewCharacterMesh)
	{
		PreviewCharacterMesh->SetSkeletalMesh(GetResolvedDefaultPreviewMesh());
		ApplyPreviewCharacterMaterials(GetResolvedDefaultPreviewMesh(), {});
	}

	if (bLockedIn && DefaultReadyPreviewAnimClass)
	{
		ApplyPreviewAnimClass(DefaultReadyPreviewAnimClass);
	}
	else
	{
		ApplyPreviewAnimClass(DefaultPreviewAnimClass);
	}
}

void AClassSelectionPreviewActor::ApplyClassPreview(UCharacterClassInfo* SelectedClass, bool bLockedIn)
{
	if (!PreviewCharacterMesh)
	{
		return;
	}

	if (!SelectedClass)
	{
		PreviewCharacterMesh->SetSkeletalMesh(nullptr);
		ApplyPreviewCharacterMaterials(nullptr, {});
		ApplyPreviewAnimClass(nullptr);
		SetPreviewVisible(false);
		ClearWeaponPreview();
		return;
	}

	SetPreviewVisible(true);
	USkeletalMesh* SelectedCharacterMesh = SelectedClass->GetResolvedCharacterMesh();
	PreviewCharacterMesh->SetSkeletalMesh(SelectedCharacterMesh);
	ApplyPreviewCharacterMaterials(SelectedCharacterMesh, SelectedClass->CharacterData.CharacterMaterialOverrides);
	ApplyPreviewAnimClass(bLockedIn && DefaultReadyPreviewAnimClass
		? DefaultReadyPreviewAnimClass
		: DefaultPreviewAnimClass);
}

void AClassSelectionPreviewActor::ApplyWeaponPreview(TSubclassOf<ANexusWeaponBase> SelectedWeaponClass)
{
	if (!PreviewCharacterMesh)
	{
		return;
	}

	if (!SelectedWeaponClass)
	{
		ClearWeaponPreview();
		return;
	}

	const ANexusWeaponBase* WeaponCDO = SelectedWeaponClass->GetDefaultObject<ANexusWeaponBase>();
	if (!WeaponCDO)
	{
		ClearWeaponPreview();
		return;
	}

	const FWeaponConfig& WeaponConfig = WeaponCDO->WeaponConfig;

	if (const UStaticMeshComponent* SourceWeaponMesh = WeaponCDO->GetWeaponMesh())
	{
		CopyStaticMeshComponentVisuals(SourceWeaponMesh, PreviewWeaponMesh);

		if (PreviewWeaponMesh)
		{
			PreviewWeaponMesh->AttachToComponent(
				PreviewCharacterMesh,
				FAttachmentTransformRules::SnapToTargetIncludingScale,
				WeaponConfig.WeaponSocketName);

			PreviewWeaponMesh->SetVisibility(SourceWeaponMesh->GetStaticMesh() != nullptr, true);
			PreviewWeaponMesh->SetHiddenInGame(SourceWeaponMesh->GetStaticMesh() == nullptr, true);
		}
	}
	else if (PreviewWeaponMesh)
	{
		PreviewWeaponMesh->SetStaticMesh(nullptr);
		PreviewWeaponMesh->SetHiddenInGame(true, true);
	}

	if (WeaponCDO->HasOffHandWeapon())
	{
		if (const UStaticMeshComponent* SourceOffHandMesh = WeaponCDO->GetOffHandWeaponMesh())
		{
			CopyStaticMeshComponentVisuals(SourceOffHandMesh, PreviewOffHandWeaponMesh);

			if (PreviewOffHandWeaponMesh)
			{
				PreviewOffHandWeaponMesh->AttachToComponent(
					PreviewCharacterMesh,
					FAttachmentTransformRules::SnapToTargetIncludingScale,
					WeaponConfig.OffHandSocketName);

				PreviewOffHandWeaponMesh->SetVisibility(SourceOffHandMesh->GetStaticMesh() != nullptr, true);
				PreviewOffHandWeaponMesh->SetHiddenInGame(SourceOffHandMesh->GetStaticMesh() == nullptr, true);
			}
		}
	}
	else if (PreviewOffHandWeaponMesh)
	{
		PreviewOffHandWeaponMesh->SetStaticMesh(nullptr);
		PreviewOffHandWeaponMesh->SetHiddenInGame(true, true);
	}
	ApplyPreviewAnimClass(WeaponConfig.AnimInstanceClass);
}

void AClassSelectionPreviewActor::ClearWeaponPreview()
{
	if (PreviewWeaponMesh)
	{
		PreviewWeaponMesh->SetStaticMesh(nullptr);
		PreviewWeaponMesh->SetHiddenInGame(true, true);
		PreviewWeaponMesh->SetVisibility(false, true);
	}

	if (PreviewOffHandWeaponMesh)
	{
		PreviewOffHandWeaponMesh->SetStaticMesh(nullptr);
		PreviewOffHandWeaponMesh->SetHiddenInGame(true, true);
		PreviewOffHandWeaponMesh->SetVisibility(false, true);
	}
}

void AClassSelectionPreviewActor::SetPreviewVisible(bool bVisible)
{
	SetActorHiddenInGame(!bVisible);

	if (PreviewCharacterMesh)
	{
		PreviewCharacterMesh->SetVisibility(bVisible, true);
		PreviewCharacterMesh->SetHiddenInGame(!bVisible, true);
	}
}

void AClassSelectionPreviewActor::ApplyPreviewAnimClass(TSubclassOf<UAnimInstance> InAnimClass)
{
	if (!PreviewCharacterMesh)
	{
		return;
	}

	if (InAnimClass)
	{
		PreviewCharacterMesh->SetAnimationMode(EAnimationMode::AnimationBlueprint);
		PreviewCharacterMesh->SetAnimInstanceClass(InAnimClass);
	}
	else
	{
		PreviewCharacterMesh->SetAnimInstanceClass(nullptr);
	}
}

USkeletalMesh* AClassSelectionPreviewActor::GetResolvedDefaultPreviewMesh() const
{
	return DefaultPreviewMesh;
}

void AClassSelectionPreviewActor::ApplyPreviewCharacterMaterials(
	USkeletalMesh* SourceMesh,
	const TArray<TObjectPtr<UMaterialInterface>>& MaterialOverrides)
{
	if (!PreviewCharacterMesh)
	{
		return;
	}

	const int32 SourceMaterialCount = SourceMesh ? SourceMesh->GetMaterials().Num() : 0;
	const int32 TotalMaterialCount = FMath::Max(SourceMaterialCount, PreviewCharacterMesh->GetNumMaterials());

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

		PreviewCharacterMesh->SetMaterial(MaterialIndex, DesiredMaterial);
	}
}

void AClassSelectionPreviewActor::CopyStaticMeshComponentVisuals(
	const UStaticMeshComponent* SourceMeshComponent,
	UStaticMeshComponent* TargetMeshComponent)
{
	if (!SourceMeshComponent || !TargetMeshComponent)
	{
		return;
	}

	TargetMeshComponent->SetStaticMesh(SourceMeshComponent->GetStaticMesh());
	TargetMeshComponent->SetRelativeTransform(SourceMeshComponent->GetRelativeTransform());

	const int32 NumMaterials = SourceMeshComponent->GetNumMaterials();
	for (int32 MaterialIndex = 0; MaterialIndex < NumMaterials; ++MaterialIndex)
	{
		TargetMeshComponent->SetMaterial(MaterialIndex, SourceMeshComponent->GetMaterial(MaterialIndex));
	}
}
