// NexusCapturePoint.cpp
#include "Nexus/GameMode/CapturePoints/CapturePoint.h"

#include "Components/SphereComponent.h"
#include "Nexus/GameMode/NexusGameMode.h"
#include "Nexus/Character/NexusCharacterBase.h"
#include "Nexus/Character/Enemy/Minions/NexusMinionBase.h"

ANexusCapturePoint::ANexusCapturePoint()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	CapturePointMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CapturePointMesh"));
	CapturePointMesh->SetupAttachment(GetRootComponent());

	CaptureArea = CreateDefaultSubobject<USphereComponent>(TEXT("CaptureArea"));
	CaptureArea->SetupAttachment(Root);
	CaptureArea->SetSphereRadius(300.f);
	CaptureArea->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CaptureArea->SetCollisionResponseToAllChannels(ECR_Ignore);
	CaptureArea->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

void ANexusCapturePoint::BeginPlay()
{
	Super::BeginPlay();

	MaterialInstance = CapturePointMesh->CreateDynamicMaterialInstance(0);

	if (bAutoRegisterFromOverlap)
	{
		CaptureArea->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnCaptureAreaBeginOverlap);
		CaptureArea->OnComponentEndOverlap.AddDynamic(this, &ThisClass::OnCaptureAreaEndOverlap);
	}
}

void ANexusCapturePoint::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	EvaluateControl(DeltaTime);
}

void ANexusCapturePoint::RegisterCombatUnit(ANexusCharacterBase* Unit)
{
	if (!IsValid(Unit))
	{
		return;
	}

	const ENexusTeamID TeamID = Unit->GetTeamID();
	if (TeamID == ENexusTeamID::Neutral)
	{
		return;
	}


	if (ANexusMinionBase* Minion = Cast<ANexusMinionBase>(Unit))
	{
		if (TeamID == ENexusTeamID::TeamA)
		{
			TeamAMinions.AddUnique(Minion);
		}
		else if (TeamID == ENexusTeamID::TeamB)
		{
			TeamBMinions.AddUnique(Minion);
		}
		return;
	}
	if (TeamID == ENexusTeamID::TeamA)
	{
		TeamAPlayers.AddUnique(Unit);
	}
	else if (TeamID == ENexusTeamID::TeamB)
	{
		TeamBPlayers.AddUnique(Unit);
	}
}

void ANexusCapturePoint::UnregisterCombatUnit(ANexusCharacterBase* Unit)
{
	if (!IsValid(Unit))
	{
		return;
	}

	if (ANexusMinionBase* Minion = Cast<ANexusMinionBase>(Unit))
	{
		TeamAMinions.Remove(Minion);
		TeamBMinions.Remove(Minion);
	}
	TeamAPlayers.Remove(Unit);
	TeamBPlayers.Remove(Unit);
}

bool ANexusCapturePoint::IsMinionInside(ANexusMinionBase* Minion) const
{
	return TeamAMinions.Contains(Minion) || TeamBMinions.Contains(Minion);
}

ANexusCharacterBase* ANexusCapturePoint::FindClosestEnemyFor(ANexusCharacterBase* RequestingUnit) const
{
	if (!IsValid(RequestingUnit))
	{
		return nullptr;
	}

	ANexusCharacterBase* BestTarget = nullptr;
	float BestDistanceSq = TNumericLimits<float>::Max();

	const FVector RequesterLocation = RequestingUnit->GetActorLocation();

	auto EvaluateCharacterArray = [&](const TArray<TObjectPtr<ANexusCharacterBase>>& Candidates)
	{
		for (ANexusCharacterBase* Candidate : Candidates)
		{
			if (!IsValid(Candidate) || Candidate->bIsDead)
			{
				continue;
			}

			if (!RequestingUnit->IsEnemyTo(Candidate))
			{
				continue;
			}

			const float DistSq = FVector::DistSquared(RequesterLocation, Candidate->GetActorLocation());
			if (DistSq < BestDistanceSq)
			{
				BestDistanceSq = DistSq;
				BestTarget = Candidate;
			}
		}
	};

	auto EvaluateMinionArray = [&](const TArray<TObjectPtr<ANexusMinionBase>>& Candidates)
	{
		for (ANexusMinionBase* Candidate : Candidates)
		{
			if (!IsValid(Candidate) || Candidate->bIsDead)
			{
				continue;
			}

			if (!RequestingUnit->IsEnemyTo(Candidate))
			{
				continue;
			}

			const float DistSq = FVector::DistSquared(RequesterLocation, Candidate->GetActorLocation());
			if (DistSq < BestDistanceSq)
			{
				BestDistanceSq = DistSq;
				BestTarget = Candidate;
			}
		}
	};

	switch (RequestingUnit->GetTeamID())
	{
	case ENexusTeamID::TeamA:
		EvaluateMinionArray(TeamBMinions);
		EvaluateCharacterArray(TeamBPlayers);
		break;

	case ENexusTeamID::TeamB:
		EvaluateMinionArray(TeamAMinions);
		EvaluateCharacterArray(TeamAPlayers);
		break;

	default:
		break;
	}

	return BestTarget;
}

void ANexusCapturePoint::SetCapturePointMaterial(ENexusTeamID TeamID) const
{
	if (CapturePointMesh)
	{
		switch (TeamID)
		{
		case ENexusTeamID::TeamA:
			MaterialInstance->SetVectorParameterValue(FName("GlowColor"), TeamAColor);
			break;

		case ENexusTeamID::TeamB:
			MaterialInstance->SetVectorParameterValue(FName("GlowColor"), TeamBColor);
			break;
		default:
			if (bIsContested)
			{
				MaterialInstance->SetVectorParameterValue(FName("GlowColor"), ContestedColor);
			}
			else
			{
				MaterialInstance->SetVectorParameterValue(FName("GlowColor"), UncontestedColor);
			}
		}
	}
}

void ANexusCapturePoint::OnCaptureAreaBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	ANexusCharacterBase* Unit = Cast<ANexusCharacterBase>(OtherActor);
	if (Unit)
	{
		RegisterCombatUnit(Unit);
		if (ANexusMinionBase* Minion = Cast<ANexusMinionBase>(Unit))
		{
			Minion->HandleReachedCapturePoint(this);
		}
		else
		{
			Unit->CurrentCapturePoint = this;
		}
	}
}

void ANexusCapturePoint::OnCaptureAreaEndOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex)
{
	ANexusCharacterBase* Unit = Cast<ANexusCharacterBase>(OtherActor);
	if (Unit)
	{
		UnregisterCombatUnit(Unit);
		if (ANexusMinionBase* Minion = Cast<ANexusMinionBase>(Unit))
		{
			Minion->HandleLeftCapturePoint(this);
		}
		else
		{
			Unit->CurrentCapturePoint = nullptr;
		}
	}
}

void ANexusCapturePoint::CleanupInvalidMinions()
{
	auto CleanArray = [](TArray<TObjectPtr<ANexusMinionBase>>& Array)
	{
		Array.RemoveAll([](const TObjectPtr<ANexusMinionBase>& Minion)
		{
			return !IsValid(Minion) || Minion->bIsDead;
		});
	};

	CleanArray(TeamAMinions);
	CleanArray(TeamBMinions);
}

int32 ANexusCapturePoint::GetValidMinionCountForTeam(ENexusTeamID TeamID) const
{
	const TArray<TObjectPtr<ANexusMinionBase>>& TeamArray = GetMinionArrayForTeam(TeamID);

	int32 Count = 0;
	for (ANexusMinionBase* Minion : TeamArray)
	{
		if (IsValid(Minion) && !Minion->bIsDead)
		{
			++Count;
		}
	}
	return Count;
}

TArray<TObjectPtr<ANexusMinionBase>>& ANexusCapturePoint::GetMinionArrayForTeam(ENexusTeamID TeamID)
{
	if (TeamID == ENexusTeamID::TeamA)
	{
		return TeamAMinions;
	}
	return TeamBMinions;
}

const TArray<TObjectPtr<ANexusMinionBase>>& ANexusCapturePoint::GetMinionArrayForTeam(ENexusTeamID TeamID) const
{
	if (TeamID == ENexusTeamID::TeamA)
	{
		return TeamAMinions;
	}
	return TeamBMinions;
}

void ANexusCapturePoint::EvaluateControl(float DeltaTime)
{
	CleanupInvalidMinions();

	const int32 TeamACount = GetValidMinionCountForTeam(ENexusTeamID::TeamA);
	const int32 TeamBCount = GetValidMinionCountForTeam(ENexusTeamID::TeamB);

	if (TeamACount > 0 && TeamBCount > 0)
	{
		bIsContested = true;
		CurrentResolvingTeam = ENexusTeamID::Neutral;
		CurrentUncontestedTime = 0.f;

		SetCapturePointMaterial(ENexusTeamID::Neutral);
		return;
	}

	if (TeamACount <= 0 && TeamBCount <= 0)
	{
		bIsContested = false;
		CurrentResolvingTeam = ENexusTeamID::Neutral;
		CurrentUncontestedTime = 0.f;

		SetCapturePointMaterial(ENexusTeamID::Neutral);
		return;
	}

	bIsContested = false;

	const ENexusTeamID NewResolvingTeam = (TeamACount > 0) ? ENexusTeamID::TeamA : ENexusTeamID::TeamB;

	if (CurrentResolvingTeam != NewResolvingTeam)
	{
		CurrentResolvingTeam = NewResolvingTeam;
		CurrentUncontestedTime = 0.f;
		
		SetCapturePointMaterial(NewResolvingTeam);
	}

	CurrentUncontestedTime += DeltaTime;

	if (CurrentUncontestedTime >= RequiredUncontestedTime)
	{
		ResolveCapture(CurrentResolvingTeam);
	}
}

void ANexusCapturePoint::ResolveCapture(ENexusTeamID ScoringTeam)
{
	TArray<TObjectPtr<ANexusMinionBase>>& ScoringMinions = GetMinionArrayForTeam(ScoringTeam);
	const int32 ScoreAmount = GetValidMinionCountForTeam(ScoringTeam);

	if (ScoreAmount <= 0)
	{
		CurrentResolvingTeam = ENexusTeamID::Neutral;
		CurrentUncontestedTime = 0.f;
		return;
	}

	if (ANexusGameMode* GM = GetWorld()->GetAuthGameMode<ANexusGameMode>())
	{
		GM->AddScoreForTeam(ScoringTeam, ScoreAmount);
	}

	TArray<TObjectPtr<ANexusMinionBase>> MinionsToDestroy = ScoringMinions;

	for (ANexusMinionBase* Minion : MinionsToDestroy)
	{
		if (IsValid(Minion))
		{
			Minion->Destroy();
		}
	}

	TeamAMinions.Empty();
	TeamBMinions.Empty();

	CurrentResolvingTeam = ENexusTeamID::Neutral;
	CurrentUncontestedTime = 0.f;
	bIsContested = false;

	BP_OnResolveCapture();
	SetCapturePointMaterial(ENexusTeamID::Neutral);
}