#pragma once

#include "CoreMinimal.h"
#include "NexusGameplayAbility.h"
#include "GA_ProjectileExplode.generated.h"

struct FGameplayEventData;
struct FHitResult;

UCLASS()
class NEXUS_API UGA_ProjectileExplode : public UNexusGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_ProjectileExplode();

protected:
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
		class ANexusCharacterBase* SourceCharacter,
		float DamageAmount,
		const FHitResult* HitResult) const;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Explosion")
	float ExplosionRadius = 250.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Explosion")
	float FallbackDamage = 10.f;
};