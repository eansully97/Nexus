#include "GA_Axe_Swing.h"

#include "Nexus/NexusGameplayTags.h"

UGA_Axe_Swing::UGA_Axe_Swing()
{
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(NexusGameplayTags::Ability_Weapon_Axe_Swing);
	SetAssetTags(AssetTags);
}
