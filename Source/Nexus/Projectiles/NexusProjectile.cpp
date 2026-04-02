// NexusProjectile.cpp

#include "NexusProjectile.h"

#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Net/UnrealNetwork.h"

ANexusProjectile::ANexusProjectile()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	SetRootComponent(CollisionSphere);
	CollisionSphere->SetSphereRadius(16.f);
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionSphere->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionSphere->SetCollisionResponseToAllChannels(ECR_Block);
	CollisionSphere->SetNotifyRigidBodyCollision(true);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->InitialSpeed = 0.f;
	ProjectileMovement->MaxSpeed = 10000.f;
	ProjectileMovement->ProjectileGravityScale = 0.f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false;
}

void ANexusProjectile::BeginPlay()
{
	Super::BeginPlay();

	// Safety: if the projectile was replicated to a client after being initialized,
	// make sure movement velocity matches replicated data.
	if (ProjectileMovement && !ReplicatedInitialDirection.IsNearlyZero() && ReplicatedInitialSpeed > 0.f)
	{
		ProjectileMovement->Velocity = ReplicatedInitialDirection * ReplicatedInitialSpeed;
	}
}

void ANexusProjectile::InitializeProjectile(
	AActor* InSourceActor,
	UAbilitySystemComponent* InSourceASC,
	const FVector& InDirection,
	float InSpeed,
	float InDamage)
{
	SourceActor = InSourceActor;
	SourceASC = InSourceASC;
	Damage = InDamage;

	ReplicatedInitialDirection = InDirection.GetSafeNormal();
	ReplicatedInitialSpeed = InSpeed;

	if (ProjectileMovement)
	{
		ProjectileMovement->InitialSpeed = ReplicatedInitialSpeed;
		ProjectileMovement->MaxSpeed = ReplicatedInitialSpeed;
		ProjectileMovement->Velocity = ReplicatedInitialDirection * ReplicatedInitialSpeed;
	}
}


void ANexusProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANexusProjectile, SourceActor);
	DOREPLIFETIME(ANexusProjectile, ReplicatedInitialDirection);
	DOREPLIFETIME(ANexusProjectile, ReplicatedInitialSpeed);
}