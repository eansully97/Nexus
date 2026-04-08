#include "NexusMinionBase.h"

#include "Animation/AnimInstance.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Net/UnrealNetwork.h"
#include "Nexus/Character/NexusCharacterBase.h"
#include "Nexus/GameMode/CapturePoints/CapturePoint.h"
#include "Nexus/GameplayAbilitySystem/Abilities/GA_Minion_Attack.h"

namespace
{
	const TCHAR* TeamAMeshPath =
		TEXT("/Game/ParagonMinions/Characters/Minions/Down_Minions/Meshes/Minion_Lane_Melee_Dawn.Minion_Lane_Melee_Dawn");
	const TCHAR* TeamBMeshPath =
		TEXT("/Game/ParagonMinions/Characters/Minions/Dusk_Minions/Meshes/Minion_Lane_Melee_Dusk.Minion_Lane_Melee_Dusk");
	const TCHAR* MinionAnimClassPath =
		TEXT("/Game/Nexus/Characters/Enemies/Animations/ABP_Minion.ABP_Minion_C");
}

ANexusMinionBase::ANexusMinionBase()
{
	PrimaryActorTick.bCanEverTick = false;
	DefaultAttackAbilityClass = UGA_Minion_Attack::StaticClass();
}

void ANexusMinionBase::BeginPlay()
{
	Super::BeginPlay();
	EnsureDefaultVisualSetup();

	if (HasAuthority())
	{
		StartTargetScan();
	}
}

void ANexusMinionBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, CurrentTarget);
}

void ANexusMinionBase::InitializeCombatLoadout()
{
	if (!HasAuthority())
	{
		return;
	}

	// PossessedBy() on the base may call this before InitializeMinion().
	// For minions, wait until the server has assigned the capture point/team/setup data.
	if (!bMinionLoadoutInitialized)
	{
		return;
	}

	Super::InitializeCombatLoadout();
}

TArray<FNexusAbilityGrant> ANexusMinionBase::GetClassAbilitiesToGrant() const
{
	TArray<FNexusAbilityGrant> Result = AdditionalMinionAbilityGrants;

	if (DefaultAttackAbilityClass)
	{
		const bool bAlreadyHasDefaultAttack =
			Result.ContainsByPredicate(
				[this](const FNexusAbilityGrant& Grant)
				{
					return Grant.Ability == DefaultAttackAbilityClass;
				});

		if (!bAlreadyHasDefaultAttack)
		{
			FNexusAbilityGrant DefaultAttackGrant;
			DefaultAttackGrant.Ability = DefaultAttackAbilityClass;
			DefaultAttackGrant.AbilityLevel = FMath::Max(DefaultAttackAbilityLevel, 1);
			Result.Add(DefaultAttackGrant);
		}
	}

	return Result;
}

void ANexusMinionBase::OnRep_CurrentTarget()
{
	OnCombatStateChanged.Broadcast();
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

	Hitscan();

	GetWorld()->GetTimerManager().SetTimer(
		HitscanTimerHandle,
		this,
		&ThisClass::Hitscan,
		HitscanInterval,
		true
	);
	RotateTowardsCurrentTargetForHitscan(HitscanInterval);
}

void ANexusMinionBase::EndHitscan()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(HitscanTimerHandle);
	}

	AlreadyHitCharactersInWindow.Reset();
}

bool ANexusMinionBase::IsFacingActorForHitscan(const AActor* ActorToFace) const
{
	if (!ActorToFace)
	{
		return false;
	}

	FVector ToTarget = ActorToFace->GetActorLocation() - GetActorLocation();
	ToTarget.Z = 0.f;

	if (!ToTarget.Normalize())
	{
		return true;
	}

	const FVector Forward = GetActorForwardVector().GetSafeNormal2D();
	const float FacingDot = FVector::DotProduct(Forward, ToTarget);

	return FacingDot >= AttackFacingDotThreshold;
}

bool ANexusMinionBase::RotateTowardsCurrentTargetForHitscan(float DeltaTime)
{
	if (!HasAuthority())
	{
		return false;
	}

	ANexusCharacterBase* TargetCharacter = Cast<ANexusCharacterBase>(CurrentTarget);
	if (!IsValid(TargetCharacter) || TargetCharacter == this || TargetCharacter->GetIsDead() || !IsEnemyTo(TargetCharacter))
	{
		return true;
	}

	FVector ToTarget = TargetCharacter->GetActorLocation() - GetActorLocation();
	ToTarget.Z = 0.f;

	if (!ToTarget.Normalize())
	{
		return true;
	}

	FRotator CurrentRotation = GetActorRotation();
	CurrentRotation.Pitch = 0.f;
	CurrentRotation.Roll = 0.f;

	FRotator DesiredRotation = ToTarget.Rotation();
	DesiredRotation.Pitch = 0.f;
	DesiredRotation.Roll = 0.f;

	const FRotator NewRotation = FMath::RInterpConstantTo(
		CurrentRotation,
		DesiredRotation,
		FMath::Max(DeltaTime, KINDA_SMALL_NUMBER),
		AttackTurnRateDegreesPerSecond
	);

	SetActorRotation(NewRotation);

	if (Controller)
	{
		Controller->SetControlRotation(NewRotation);
	}

	return IsFacingActorForHitscan(TargetCharacter);
}

void ANexusMinionBase::Hitscan()
{
	if (!HasAuthority())
	{
		return;
	}

	const float TurnDeltaTime =
		HitscanInterval > 0.f
			? HitscanInterval
			: (GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.03f);

	if (!RotateTowardsCurrentTargetForHitscan(TurnDeltaTime))
	{
		return;
	}

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
		ResolveMeleeHit(Character, HitResult, DamageToDeal);
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

	bMinionLoadoutInitialized = true;
	RebuildCombatLoadout();

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

	ANexusMinionBase* MutableThis = const_cast<ANexusMinionBase*>(this);
	MutableThis->EnsureDefaultVisualSetup();

	if (!GetMesh())
	{
		return;
	}

	if (USkeletalMesh* DesiredMesh = MutableThis->GetDesiredTeamMesh())
	{
		GetMesh()->SetSkeletalMeshAsset(DesiredMesh);
	}
}

void ANexusMinionBase::EnsureDefaultVisualSetup()
{
	if (!TeamAMesh)
	{
		TeamAMesh = LoadObject<USkeletalMesh>(nullptr, TeamAMeshPath);
	}

	if (!TeamBMesh)
	{
		TeamBMesh = LoadObject<USkeletalMesh>(nullptr, TeamBMeshPath);
	}

	if (!MinionAnimClass)
	{
		MinionAnimClass = LoadClass<UAnimInstance>(nullptr, MinionAnimClassPath);
	}

	USkeletalMeshComponent* MeshComp = GetMesh();
	if (!MeshComp)
	{
		return;
	}

	if (!MeshComp->GetSkeletalMeshAsset())
	{
		if (USkeletalMesh* DesiredMesh = GetDesiredTeamMesh())
		{
			MeshComp->SetSkeletalMeshAsset(DesiredMesh);
		}
	}

	if (MinionAnimClass && MeshComp->GetAnimClass() != MinionAnimClass)
	{
		MeshComp->SetAnimationMode(EAnimationMode::AnimationBlueprint);
		MeshComp->SetAnimInstanceClass(MinionAnimClass);
	}
}

USkeletalMesh* ANexusMinionBase::GetDesiredTeamMesh() const
{
	switch (TeamID)
	{
	case ENexusTeamID::TeamB:
		return TeamBMesh ? TeamBMesh.Get() : TeamAMesh.Get();

	case ENexusTeamID::TeamA:
	case ENexusTeamID::Neutral:
	default:
		return TeamAMesh ? TeamAMesh.Get() : TeamBMesh.Get();
	}
}

void ANexusMinionBase::StartTargetScan()
{
	if (!HasAuthority() || !GetWorld())
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
	if (!HasAuthority() || !GetWorld())
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
			if (Dot < -0.7f)
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

	ForceNetUpdate();
	OnCombatStateChanged.Broadcast();
}
