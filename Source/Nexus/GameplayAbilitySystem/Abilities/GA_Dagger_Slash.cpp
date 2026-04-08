#include "GA_Dagger_Slash.h"

#include "Nexus/NexusGameplayTags.h"

UGA_Dagger_Slash::UGA_Dagger_Slash()
{
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(NexusGameplayTags::Ability_Weapon_Dagger_Slash);
	SetAssetTags(AssetTags);
}
