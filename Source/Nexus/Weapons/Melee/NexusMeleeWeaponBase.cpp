#include "NexusMeleeWeaponBase.h"

#include "GameplayCueFunctionLibrary.h"
#include "Components/StaticMeshComponent.h"
#include "Nexus/Character/NexusCharacterBase.h"

void ANexusMeleeWeaponBase::BeginPlay()
{
	Super::BeginPlay();
	RefreshOwnerCharacter();
}

bool ANexusMeleeWeaponBase::RefreshOwnerCharacter()
{
	if (IsValid(OwnerCharacter))
	{
		return true;
	}

	OwnerCharacter = Cast<ANexusCharacterBase>(GetOwner());
	return IsValid(OwnerCharacter);
}

void ANexusMeleeWeaponBase::StartHitscan()
{
	if (!GetWorld())
	{
		return;
	}

	if (!RefreshOwnerCharacter())
	{
		return;
	}

	AlreadyHitCharactersInWindow.Reset();

	// Immediate first trace so the hit window starts right away.
	Hitscan();

	// Keep tracing during the active window until EndHitscan is called.
	if (!GetWorld()->GetTimerManager().IsTimerActive(HitscanTimerHandle))
	{
		GetWorld()->GetTimerManager().SetTimer(
			HitscanTimerHandle,
			this,
			&ThisClass::Hitscan,
			HitscanInterval,
			true
		);
	}
}

void ANexusMeleeWeaponBase::EndHitscan()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(HitscanTimerHandle);
	}

	AlreadyHitCharactersInWindow.Reset();
}

void ANexusMeleeWeaponBase::Hitscan()
{
	DoHitscan();
}

void ANexusMeleeWeaponBase::DoHitscan()
{
	if (!RefreshOwnerCharacter())
	{
		return;
	}

	if (WeaponOrientation == EWeaponOrientation::MainHand)
	{
		if (!WeaponMesh)
		{
			return;
		}

		ProcessWeaponTrace(WeaponMesh, AlreadyHitCharactersInWindow);
		return;
	}

	if (WeaponOrientation == EWeaponOrientation::OffHand)
	{
		if (!OffHandWeaponMesh)
		{
			return;
		}

		ProcessWeaponTrace(OffHandWeaponMesh, AlreadyHitCharactersInWindow);
		return;
	}

	if (WeaponOrientation == EWeaponOrientation::Both)
	{
		if (WeaponMesh)
		{
			ProcessWeaponTrace(WeaponMesh, AlreadyHitCharactersInWindow);
		}

		if (OffHandWeaponMesh)
		{
			ProcessWeaponTrace(OffHandWeaponMesh, AlreadyHitCharactersInWindow);
		}

		return;
	}
}

void ANexusMeleeWeaponBase::ProcessWeaponTrace(
	const UStaticMeshComponent* MeshToTrace,
	TArray<TObjectPtr<ANexusCharacterBase>>& AlreadyHitCharacters) const
{
	if (!IsValid(OwnerCharacter) || !IsValid(MeshToTrace))
	{
		return;
	}

	if (!MeshToTrace->DoesSocketExist(TEXT("TraceStart")) ||
		!MeshToTrace->DoesSocketExist(TEXT("TraceEnd")))
	{
		return;
	}

	TArray<FHitResult> ValidHitResults;

	const FVector HitscanStartLoc = MeshToTrace->GetSocketLocation(TEXT("TraceStart"));
	const FVector HitscanEndLoc = MeshToTrace->GetSocketLocation(TEXT("TraceEnd"));

	TArray<ANexusCharacterBase*> CharactersToEffect =
		OwnerCharacter->PerformSphereTraceForValidEnemies(
			HitscanStartLoc,
			HitscanEndLoc,
			DamageHitScanRadius,
			ValidHitResults);

	const int32 NumPairs = FMath::Min(CharactersToEffect.Num(), ValidHitResults.Num());

	for (int32 Index = 0; Index < NumPairs; ++Index)
	{
		ANexusCharacterBase* Character = CharactersToEffect[Index];
		const FHitResult& HitResult = ValidHitResults[Index];

		if (!IsValid(Character) || AlreadyHitCharacters.Contains(Character))
		{
			continue;
		}

		AlreadyHitCharacters.Add(Character);

		OwnerCharacter->ApplyDamageToTarget(Character, DamageToDeal);

		FGameplayCueParameters Parameters;
		Parameters.Location = HitResult.ImpactPoint;
		Parameters.Normal = HitResult.ImpactNormal;
		Parameters.Instigator = OwnerCharacter;

		UGameplayCueFunctionLibrary::ExecuteGameplayCueOnActor(
			Character,
			FGameplayTag::RequestGameplayTag(TEXT("GameplayCue.Damage.Burst")),
			Parameters);
	}
}