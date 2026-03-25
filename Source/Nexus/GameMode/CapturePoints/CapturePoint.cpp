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

void ANexusCapturePoint::RegisterMinion(ANexusMinionBase* Minion)
{
	if (!IsValid(Minion))
	{
		return;
	}

	const ENexusTeamID TeamID = Minion->GetTeamID();
	if (TeamID == ENexusTeamID::Neutral)
	{
		return;
	}

	TArray<TObjectPtr<ANexusMinionBase>>& TeamArray = GetMinionArrayForTeam(TeamID);
	if (!TeamArray.Contains(Minion))
	{
		TeamArray.Add(Minion);
	}
}

void ANexusCapturePoint::UnregisterMinion(ANexusMinionBase* Minion)
{
	if (!IsValid(Minion))
	{
		return;
	}

	TeamAMinions.Remove(Minion);
	TeamBMinions.Remove(Minion);
}

bool ANexusCapturePoint::IsMinionInside(ANexusMinionBase* Minion) const
{
	return TeamAMinions.Contains(Minion) || TeamBMinions.Contains(Minion);
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
	ANexusMinionBase* Minion = Cast<ANexusMinionBase>(OtherActor);
	if (Minion)
	{
		RegisterMinion(Minion);
		Minion->HandleReachedCapturePoint(this);
	}
}

void ANexusCapturePoint::OnCaptureAreaEndOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex)
{
	ANexusMinionBase* Minion = Cast<ANexusMinionBase>(OtherActor);
	if (Minion)
	{
		UnregisterMinion(Minion);
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

	SetCapturePointMaterial(ENexusTeamID::Neutral);
}