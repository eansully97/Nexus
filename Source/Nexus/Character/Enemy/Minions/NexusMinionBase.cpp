#include "NexusMinionBase.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Nexus/Character/NexusCharacterBase.h"
#include "Nexus/GameMode/CapturePoints/CapturePoint.h"
#include "Nexus/GameplayAbilitySystem/Abilities/NexusGameplayAbility.h"

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

	SetCurrentCapturePoint(CapturePoint);

	if (AAIController* AI = Cast<AAIController>(GetController()))
	{
		AI->StopMovement();

		if (UBlackboardComponent* BB = AI->GetBlackboardComponent())
		{
			BB->SetValueAsBool(TEXT("AtCapturePoint"), true);
			BB->SetValueAsObject(TEXT("CapturePoint"), CapturePoint);
		}
	}
}

void ANexusMinionBase::HandleLeftCapturePoint(ANexusCapturePoint* CapturePoint)
{
	if (!HasAuthority() || CapturePoint != GetCurrentCapturePoint())
	{
		return;
	}

	SetCurrentCapturePoint(nullptr);

	if (AAIController* AI = Cast<AAIController>(GetController()))
	{
		if (UBlackboardComponent* BB = AI->GetBlackboardComponent())
		{
			BB->SetValueAsBool(TEXT("AtCapturePoint"), false);
			BB->SetValueAsObject(TEXT("CapturePoint"), TargetCapturePoint);
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

	for (AActor* Actor : OverlappedActors)
	{
		ANexusCharacterBase* Candidate = Cast<ANexusCharacterBase>(Actor);
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