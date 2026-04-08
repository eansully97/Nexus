#include "NexusProjectile.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "Nexus/NexusGameplayTags.h"

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

	if (CollisionSphere)
	{
		CollisionSphere->OnComponentHit.AddDynamic(this, &ANexusProjectile::OnCollisionHit);
	}

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

	if (InSourceActor)
	{
		SetOwner(InSourceActor);
		SetInstigator(Cast<APawn>(InSourceActor));
	}

	ReplicatedInitialDirection = InDirection.GetSafeNormal();
	ReplicatedInitialSpeed = InSpeed;

	if (CollisionSphere && InSourceActor)
	{
		CollisionSphere->IgnoreActorWhenMoving(InSourceActor, true);

		if (APawn* SourcePawn = Cast<APawn>(InSourceActor))
		{
			CollisionSphere->IgnoreActorWhenMoving(SourcePawn, true);
		}
	}

	if (ProjectileMovement)
	{
		ProjectileMovement->InitialSpeed = ReplicatedInitialSpeed;
		ProjectileMovement->MaxSpeed = ReplicatedInitialSpeed;
		ProjectileMovement->Velocity = ReplicatedInitialDirection * ReplicatedInitialSpeed;
	}
}

void ANexusProjectile::OnCollisionHit(
	UPrimitiveComponent* HitComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	FVector NormalImpulse,
	const FHitResult& Hit)
{
	HandleProjectileImpact(Hit);
}

void ANexusProjectile::HandleProjectileImpact(const FHitResult& Hit)
{
	if (!HasAuthority() || bHasImpacted)
	{
		return;
	}

	if (!Hit.bBlockingHit)
	{
		return;
	}

	AActor* HitActor = Hit.GetActor();
	if (HitActor && HitActor == SourceActor)
	{
		return;
	}

	bHasImpacted = true;

	if (ProjectileMovement)
	{
		ProjectileMovement->StopMovementImmediately();
	}

	SendExplodeGameplayEvent(Hit);
	Destroy();
}

void ANexusProjectile::SendExplodeGameplayEvent(const FHitResult& Hit) const
{
	if (!HasAuthority() || !SourceActor)
	{
		return;
	}

	FGameplayEventData Payload;
	Payload.EventTag = NexusGameplayTags::Event_Projectile_Explode;
	Payload.Instigator = SourceActor;
	Payload.Target = Hit.GetActor();
	Payload.OptionalObject = this;
	Payload.EventMagnitude = Damage;

	FGameplayAbilityTargetDataHandle TargetData;
	TargetData.Add(new FGameplayAbilityTargetData_SingleTargetHit(Hit));
	Payload.TargetData = TargetData;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
		SourceActor,
		Payload.EventTag,
		Payload
	);
}

void ANexusProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANexusProjectile, SourceActor);
	DOREPLIFETIME(ANexusProjectile, ReplicatedInitialDirection);
	DOREPLIFETIME(ANexusProjectile, ReplicatedInitialSpeed);
}
