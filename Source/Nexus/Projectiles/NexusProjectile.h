#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NexusProjectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;
class UAbilitySystemComponent;
class UPrimitiveComponent;

UCLASS()
class NEXUS_API ANexusProjectile : public AActor
{
	GENERATED_BODY()

public:
	ANexusProjectile();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USphereComponent> CollisionSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	UPROPERTY(Replicated)
	TObjectPtr<AActor> SourceActor;

	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> SourceASC;

	UPROPERTY(Replicated)
	FVector ReplicatedInitialDirection = FVector::ForwardVector;

	UPROPERTY(Replicated)
	float ReplicatedInitialSpeed = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Damage")
	float Damage = 0.f;

	UPROPERTY()
	bool bHasImpacted = false;

	virtual void BeginPlay() override;

	UFUNCTION()
	void OnCollisionHit(
		UPrimitiveComponent* HitComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		FVector NormalImpulse,
		const FHitResult& Hit);

	void HandleProjectileImpact(const FHitResult& Hit);
	void SendExplodeGameplayEvent(const FHitResult& Hit) const;

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void InitializeProjectile(
		AActor* InSourceActor,
		UAbilitySystemComponent* InSourceASC,
		const FVector& InDirection,
		float InSpeed,
		float InDamage
	);
};