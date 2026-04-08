#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ClassSelectionPreviewActor.generated.h"

class ANexusPlayerState;
class ANexusWeaponBase;
class UAnimInstance;
class UCharacterClassInfo;
class UMaterialInterface;
class USceneComponent;
class USkeletalMesh;
class USkeletalMeshComponent;
class UStaticMeshComponent;

UCLASS()
class NEXUS_API AClassSelectionPreviewActor : public AActor
{
	GENERATED_BODY()

public:
	AClassSelectionPreviewActor();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Preview")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Preview")
	TObjectPtr<USkeletalMeshComponent> PreviewCharacterMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Preview")
	TObjectPtr<UStaticMeshComponent> PreviewWeaponMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Preview")
	TObjectPtr<UStaticMeshComponent> PreviewOffHandWeaponMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Preview")
	int32 PreviewOwnerIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Preview")
	bool bHideActorWhenNoSelection = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Preview|Default")
	TObjectPtr<USkeletalMesh> DefaultPreviewMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Preview|Default")
	TSubclassOf<UAnimInstance> DefaultPreviewAnimClass = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Preview|Default")
	TSubclassOf<UAnimInstance> DefaultReadyPreviewAnimClass = nullptr;

	UPROPERTY(BlueprintReadOnly, Category="Preview")
	TObjectPtr<ANexusPlayerState> ObservedPlayerState = nullptr;

	FTimerHandle RetryBindTimerHandle;

	UFUNCTION()
	void HandleObservedPlayerProfileChanged();

	void TryBindToObservedPlayerState();
	void UnbindFromObservedPlayerState();

	void RefreshPreview();
	void ApplyClassPreview(UCharacterClassInfo* SelectedClass, bool bLockedIn);
	void ApplyWeaponPreview(TSubclassOf<ANexusWeaponBase> SelectedWeaponClass);
	void ClearWeaponPreview();
	void SetPreviewVisible(bool bVisible);
	void ApplyPreviewAnimClass(TSubclassOf<UAnimInstance> InAnimClass);
	void ApplyDefaultPreview(bool bLockedIn);
	USkeletalMesh* GetResolvedDefaultPreviewMesh() const;
	void ApplyPreviewCharacterMaterials(
		USkeletalMesh* SourceMesh,
		const TArray<TObjectPtr<UMaterialInterface>>& MaterialOverrides);

	static void CopyStaticMeshComponentVisuals(
		const UStaticMeshComponent* SourceMeshComponent,
		UStaticMeshComponent* TargetMeshComponent);
};
