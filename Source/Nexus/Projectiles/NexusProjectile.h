// NexusProjectile.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NexusProjectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;
class UAbilitySystemComponent;

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

	virtual void BeginPlay() override;


public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void InitializeProjectile(
		AActor* InSourceActor,
		UAbilitySystemComponent* InSourceASC,
		const FVector& InDirection,
		float InSpeed
	);
};