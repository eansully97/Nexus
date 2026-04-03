#pragma once

#include "CoreMinimal.h"
#include "NexusGameplayAbility.h"
#include "GA_ProjectileExplode.generated.h"

UCLASS()
class NEXUS_API UGA_ProjectileExplode : public UNexusGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_ProjectileExplode();

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Explosion")
	float ExplosionRadius = 250.f;

	/** Only used if the projectile/event does not provide a positive EventMagnitude */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Explosion")
	float FallbackDamage = 20.f;

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	bool TryExtractHitResult(const FGameplayEventData& EventData, FHitResult& OutHit) const;

	void ApplyExplosionAtLocation(
		const FVector& ExplosionOrigin,
		const FGameplayEventData& EventData,
		const FHitResult* DirectHit);

	void ApplyDamageToActor(
		AActor* TargetActor,
		AActor* SourceActor,
		float DamageAmount) const;
};