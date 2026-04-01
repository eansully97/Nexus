#include "NexusMinionBase.h"

#include "AIController.h"
#include "GameplayCueFunctionLibrary.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Nexus/Character/NexusCharacterBase.h"
#include "Nexus/GameMode/CapturePoints/CapturePoint.h"

ANexusMinionBase::ANexusMinionBase()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ANexusMinionBase::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		StartTargetScan();
	}
}

void ANexusMinionBase::StartHitscan()
{
	if (!HasAuthority() || !GetWorld())
	{
		return;
	}

	if (GetWorld()->GetTimerManager().IsTimerActive(HitscanTimerHandle))
	{
		return;
	}

	AlreadyHitCharactersInWindow.Reset();

	// Immediate first trace so the hit window begins right away.
	Hitscan();

	GetWorld()->GetTimerManager().SetTimer(
		HitscanTimerHandle,
		this,
		&ThisClass::Hitscan,
		HitscanInterval,
		true
	);
}

void ANexusMinionBase::EndHitscan()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(HitscanTimerHandle);
	}

	AlreadyHitCharactersInWindow.Reset();
}

void ANexusMinionBase::Hitscan()
{
	DoHitscan();
}

void ANexusMinionBase::DoHitscan()
{
	if (!HasAuthority())
	{
		return;
	}

	USkeletalMeshComponent* MeshComp = GetMesh();
	if (!MeshComp)
	{
		return;
	}

	if (!MeshComp->DoesSocketExist(TEXT("WeaponStartSocket")) ||
		!MeshComp->DoesSocketExist(TEXT("WeaponEndSocket")))
	{
		return;
	}

	const FVector HitscanStartLoc = MeshComp->GetSocketLocation(TEXT("WeaponStartSocket"));
	const FVector HitscanEndLoc = MeshComp->GetSocketLocation(TEXT("WeaponEndSocket"));

	TArray<FHitResult> ValidHitResults;
	TArray<ANexusCharacterBase*> CharactersToEffect = PerformSphereTraceForValidEnemies(
		HitscanStartLoc,
		HitscanEndLoc,
		DamageHitScanRadius,
		ValidHitResults);

	const int32 NumPairs = FMath::Min(CharactersToEffect.Num(), ValidHitResults.Num());

	for (int32 Index = 0; Index < NumPairs; ++Index)
	{
		ANexusCharacterBase* Character = CharactersToEffect[Index];
		const FHitResult& HitResult = ValidHitResults[Index];

		if (!IsValid(Character) || AlreadyHitCharactersInWindow.Contains(Character))
		{
			continue;
		}

		AlreadyHitCharactersInWindow.Add(Character);

		ApplyDamageToTarget(Character, DamageToDeal);

		FGameplayCueParameters Parameters;
		Parameters.Location = HitResult.ImpactPoint;
		Parameters.Normal = HitResult.ImpactNormal;
		Parameters.Instigator = this;

		UGameplayCueFunctionLibrary::ExecuteGameplayCueOnActor(
			Character,
			FGameplayTag::RequestGameplayTag(TEXT("GameplayCue.Damage.Burst")),
			Parameters);
	}
}

void ANexusMinionBase::InitializeCombatLoadout()
{
	Super::InitializeCombatLoadout();

	if (HasAuthority() && BaseAbilities.Num() > 0)
	{
		GrantAbilitySet(ENexusAbilitySource::Base, BaseAbilities);
	}
}

void ANexusMinionBase::InitializeMinion(ANexusCapturePoint* InTargetCapturePoint, ENexusTeamID InTeamID)
{
	if (!HasAuthority())
	{
		return;
	}

	TargetCapturePoint = InTargetCapturePoint;
	SetTeamID(InTeamID);
	SetCurrentCapturePoint(nullptr);

	if (AAIController* AI = Cast<AAIController>(GetController()))
	{
		if (UBlackboardComponent* BB = AI->GetBlackboardComponent())
		{
			BB->SetValueAsObject(TEXT("CapturePoint"), InTargetCapturePoint);
			BB->SetValueAsBool(TEXT("AtCapturePoint"), false);
		}
	}
}

void ANexusMinionBase::HandleReachedCapturePoint(ANexusCapturePoint* CapturePoint)
{
	if (!HasAuthority() || CapturePoint != TargetCapturePoint)
	{
		return;
	}

	bAtCapturePoint = true;
	SetCurrentCapturePoint(CapturePoint);

	if (AAIController* AI = Cast<AAIController>(GetController()))
	{
		if (UBlackboardComponent* BB = AI->GetBlackboardComponent())
		{
			BB->SetValueAsBool(TEXT("AtCapturePoint"), true);
			BB->SetValueAsObject(TEXT("CapturePoint"), CapturePoint);
		}
	}
}

void ANexusMinionBase::ApplyTeamVisuals() const
{
	Super::ApplyTeamVisuals();

	if (!GetMesh())
	{
		return;
	}

	switch (TeamID)
	{
	case ENexusTeamID::TeamA:
		if (TeamAMesh)
		{
			GetMesh()->SetSkeletalMeshAsset(TeamAMesh);
		}
		break;

	case ENexusTeamID::TeamB:
		if (TeamBMesh)
		{
			GetMesh()->SetSkeletalMeshAsset(TeamBMesh);
		}
		break;

	default:
		break;
	}
}

void ANexusMinionBase::StartTargetScan()
{
	if (!GetWorld())
	{
		return;
	}

	GetWorldTimerManager().SetTimer(
		TargetScanTimerHandle,
		this,
		&ANexusMinionBase::UpdateTargetActor,
		0.10f,
		true
	);
}

void ANexusMinionBase::StopTargetScan()
{
	if (!GetWorld())
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(TargetScanTimerHandle);
}

bool ANexusMinionBase::IsValidTarget(ANexusCharacterBase* Candidate) const
{
	if (!Candidate || Candidate == this)
	{
		return false;
	}

	if (Candidate->GetIsDead())
	{
		return false;
	}

	if (!IsEnemyTo(Candidate))
	{
		return false;
	}

	const float DistSq = FVector::DistSquared(GetActorLocation(), Candidate->GetActorLocation());
	if (DistSq > FMath::Square(AggroRange))
	{
		return false;
	}

	if (GetCurrentCapturePoint() && !IsWithinDefendLeash(Candidate))
	{
		return false;
	}

	return true;
}

bool ANexusMinionBase::IsWithinDefendLeash(const AActor* Actor) const
{
	if (!Actor)
	{
		return false;
	}

	const ANexusCapturePoint* AnchorPoint = GetCurrentCapturePoint() ? GetCurrentCapturePoint() : TargetCapturePoint;
	if (!AnchorPoint)
	{
		return true;
	}

	const float DistSq = FVector::DistSquared(AnchorPoint->GetActorLocation(), Actor->GetActorLocation());
	return DistSq <= FMath::Square(DefendLeashRadius);
}

bool ANexusMinionBase::IsInsideMyCaptureContext(const ANexusCharacterBase* Candidate) const
{
	if (!Candidate)
	{
		return false;
	}

	return GetCurrentCapturePoint() && Candidate->GetCurrentCapturePoint() == GetCurrentCapturePoint();
}

float ANexusMinionBase::ScoreTarget(ANexusCharacterBase* Candidate) const
{
	if (!IsValidTarget(Candidate))
	{
		return -FLT_MAX;
	}

	float Score = 0.f;

	if (IsInsideMyCaptureContext(Candidate))
	{
		Score += CapturePointPriorityBonus;
	}

	const float Distance = FVector::Dist(GetActorLocation(), Candidate->GetActorLocation());
	Score -= Distance;

	return Score;
}

AActor* ANexusMinionBase::FindBestTarget() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	TArray<AActor*> OverlappedActors;
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(const_cast<ANexusMinionBase*>(this));

	UKismetSystemLibrary::SphereOverlapActors(
		World,
		GetActorLocation(),
		AggroRange,
		TArray<TEnumAsByte<EObjectTypeQuery>>{UEngineTypes::ConvertToObjectType(ECC_Pawn)},
		ANexusCharacterBase::StaticClass(),
		ActorsToIgnore,
		OverlappedActors
	);

	AActor* BestActor = nullptr;
	float BestScore = -FLT_MAX;

	const FVector Forward = GetActorForwardVector();

	for (AActor* Actor : OverlappedActors)
	{
		ANexusCharacterBase* Candidate = Cast<ANexusCharacterBase>(Actor);
		if (!Candidate)
		{
			continue;
		}

		FVector ToTarget = Candidate->GetActorLocation() - GetActorLocation();
		ToTarget.Z = 0.f;

		if (!ToTarget.Normalize())
		{
			continue;
		}

		if (!bAtCapturePoint)
		{
			const float Dot = FVector::DotProduct(Forward, ToTarget);
			if (Dot < 0.34f)
			{
				continue;
			}
		}

		const float Score = ScoreTarget(Candidate);

		if (Score > BestScore)
		{
			BestScore = Score;
			BestActor = Candidate;
		}
	}

	return BestActor;
}

void ANexusMinionBase::UpdateTargetActor()
{
	if (!HasAuthority())
	{
		return;
	}

	AActor* NewTarget = FindBestTarget();
	if (CurrentTarget == NewTarget)
	{
		return;
	}

	CurrentTarget = NewTarget;

	if (AAIController* AI = Cast<AAIController>(GetController()))
	{
		if (UBlackboardComponent* BB = AI->GetBlackboardComponent())
		{
			BB->SetValueAsObject(TEXT("TargetActor"), CurrentTarget);
		}
	}
}