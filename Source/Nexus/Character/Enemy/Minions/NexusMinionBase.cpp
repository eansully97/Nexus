// Fill out your copyright notice in the Description page of Project Settings.


#include "NexusMinionBase.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Nexus/Debug/NexusDebugMacros.h"


ANexusMinionBase::ANexusMinionBase()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ANexusMinionBase::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		GetWorldTimerManager().SetTimer(
			TargetScanTimerHandle,
			this,
			&ANexusMinionBase::UpdateTargetActor,
			0.08f,
			true
		);
	}
}

void ANexusMinionBase::OnRep_TeamID()
{
	Super::OnRep_TeamID();
	ApplyTeamVisuals();
}

void ANexusMinionBase::ApplyTeamVisuals() const
{
	Super::ApplyTeamVisuals();

	switch (TeamID)
	{
	case ENexusTeamID::TeamA:
		GetMesh()->SetSkeletalMeshAsset(TeamAMesh);
		break;
	case ENexusTeamID::TeamB:
		GetMesh()->SetSkeletalMeshAsset(TeamBMesh);
		break;
	default: ;
	}
}

AActor* ANexusMinionBase::FindTargetInFront()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	TArray<AActor*> OverlappedActors;
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(this);

	UKismetSystemLibrary::SphereOverlapActors(
		World,
		GetActorLocation(),
		AggroRange,
		TArray<TEnumAsByte<EObjectTypeQuery>>{UEngineTypes::ConvertToObjectType(ECC_Pawn)},
		ANexusCharacterBase::StaticClass(),
		ActorsToIgnore,
		OverlappedActors
	);

	AActor* BestTarget = nullptr;
	float BestDistanceSq = TNumericLimits<float>::Max();

	const FVector MyLocation = GetActorLocation();
	const FVector Forward = GetActorForwardVector();

	for (AActor* Actor : OverlappedActors)
	{
		ANexusCharacterBase* Candidate = Cast<ANexusCharacterBase>(Actor);
		if (!Candidate)
		{
			continue;
		}

		if (Candidate == this)
		{
			continue;
		}

		if (!IsEnemyTo(Candidate))
		{
			continue;
		}

		if (Candidate->bIsDead)
		{
			continue;
		}

		FVector ToTarget = Candidate->GetActorLocation() - MyLocation;
		const float DistSq = ToTarget.SizeSquared();

		if (DistSq <= KINDA_SMALL_NUMBER)
		{
			continue;
		}

		ToTarget.Normalize();

		const float Dot = FVector::DotProduct(Forward, ToTarget);

		if (Dot < 0.15f)
		{
			continue;
		}

		if (DistSq < BestDistanceSq)
		{
			BestDistanceSq = DistSq;
			BestTarget = Candidate;
		}
	}

	return BestTarget;
}

bool ANexusMinionBase::IsTargetStillValid(AActor* Actor) const
{
	ANexusCharacterBase* Candidate = Cast<ANexusCharacterBase>(Actor);
	if (!Candidate)
	{
		return false;
	}

	if (Candidate == this)
	{
		return false;
	}

	if (!IsEnemyTo(Candidate))
	{
		return false;
	}

	if (Candidate->bIsDead)
	{
		return false;
	}

	const float DistSq = FVector::DistSquared(GetActorLocation(), Candidate->GetActorLocation());
	const float LoseRangeSq = LoseTargetRange * LoseTargetRange;

	if (DistSq > LoseRangeSq)
	{
		return false;
	}
	

	return true;
}

void ANexusMinionBase::UpdateTargetActor()
{
	AActor* NewTarget = CurrentTarget;

	if (!IsTargetStillValid(CurrentTarget))
	{
		NewTarget = FindTargetInFront();
	}

	if (CurrentTarget != NewTarget)
	{
		CurrentTarget = NewTarget;

		if (AAIController* AI = Cast<AAIController>(GetController()))
		{
			if (UBlackboardComponent* BB = AI->GetBlackboardComponent())
			{
				BB->SetValueAsObject(TEXT("TargetActor"), CurrentTarget);
				FGameplayEventData EventData;
				EventData.EventTag = FGameplayTag::RequestGameplayTag(FName("Event.Enemy.TargetUpdated"));
				EventData.Instigator = this;
				EventData.Target = this;

				UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, EventData.EventTag, EventData);
			}
		}
	}
}

ANexusCapturePoint* ANexusMinionBase::GetCurrentCapturePoint()
{
	if (bAtCapturePoint)
	{
		return TargetCapturePoint;
	}
	return nullptr;
}

void ANexusMinionBase::InitializeMinion(ANexusCapturePoint* InCapturePoint, ENexusTeamID InTeamID)
{
	TargetCapturePoint = InCapturePoint;
	SetTeamID(InTeamID);
	bAtCapturePoint = false;
	if (AAIController* AI = Cast<AAIController>(GetController()))
	{
		if (UBlackboardComponent* BB = AI->GetBlackboardComponent())
		{
			BB->SetValueAsObject(TEXT("CapturePoint"), InCapturePoint);
		}
	}
}

void ANexusMinionBase::HandleReachedCapturePoint(ANexusCapturePoint* CapturePoint)
{
	if (CapturePoint != TargetCapturePoint)
	{
		return;
	}

	bAtCapturePoint = true;

	if (AAIController* AI = Cast<AAIController>(GetController()))
	{
		AI->StopMovement();

		if (UBlackboardComponent* BB = AI->GetBlackboardComponent())
		{
			BB->SetValueAsBool(TEXT("AtCapturePoint"), true);
		}
	}
}