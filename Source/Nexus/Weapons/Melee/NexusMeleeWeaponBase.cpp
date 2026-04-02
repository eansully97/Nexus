#include "NexusMeleeWeaponBase.h"

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

static bool CanWeaponIssueOwningClientRPC(const ANexusCharacterBase* OwnerCharacter)
{
	return IsValid(OwnerCharacter) && OwnerCharacter->IsLocallyControlled();
}


void ANexusMeleeWeaponBase::StartHitscan()
{
	UE_LOG(LogTemp, Warning, TEXT("StartHitscan %s Role=%d"), *GetName(), (int32)GetLocalRole());
	if (HasAuthority())
	{
		StartHitscan_Internal();
		return;
	}

	if (!RefreshOwnerCharacter() || !CanWeaponIssueOwningClientRPC(OwnerCharacter))
	{
		return;
	}

	ServerStartHitscan();
}

void ANexusMeleeWeaponBase::EndHitscan()
{
	if (HasAuthority())
	{
		EndHitscan_Internal();
		return;
	}

	if (!RefreshOwnerCharacter() || !CanWeaponIssueOwningClientRPC(OwnerCharacter))
	{
		return;
	}

	ServerEndHitscan();
}

void ANexusMeleeWeaponBase::ServerStartHitscan_Implementation()
{
	StartHitscan_Internal();
}

void ANexusMeleeWeaponBase::ServerEndHitscan_Implementation()
{
	EndHitscan_Internal();
}

void ANexusMeleeWeaponBase::StartHitscan_Internal()
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

void ANexusMeleeWeaponBase::EndHitscan_Internal()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(HitscanTimerHandle);
	}

	AlreadyHitCharactersInWindow.Reset();
}

void ANexusMeleeWeaponBase::Hitscan()
{
	UE_LOG(LogTemp, Warning, TEXT("Hitscan %s Role=%d"), *GetName(), (int32)GetLocalRole());
	if (!HasAuthority())
	{
		return;
	}

	DoHitscan();
}

void ANexusMeleeWeaponBase::DoHitscan()
{
	if (!HasAuthority() || !RefreshOwnerCharacter())
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
	}
}

void ANexusMeleeWeaponBase::ProcessWeaponTrace(
	const UStaticMeshComponent* MeshToTrace,
	TArray<TObjectPtr<ANexusCharacterBase>>& AlreadyHitCharacters) const
{
	UE_LOG(LogTemp, Warning, TEXT("ProcessWeaponTrace Weapon=%s Owner=%s"),
		*GetName(),
		*GetNameSafe(OwnerCharacter));
	if (!HasAuthority() || !IsValid(OwnerCharacter) || !IsValid(MeshToTrace))
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
		UE_LOG(LogTemp, Warning, TEXT("About to ResolveMeleeHit Attacker=%s Target=%s"),
	*GetNameSafe(OwnerCharacter),
	*GetNameSafe(Character));
		OwnerCharacter->ResolveMeleeHit(Character, HitResult, DamageToDeal);
	}
}