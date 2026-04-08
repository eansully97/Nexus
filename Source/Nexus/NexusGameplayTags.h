#pragma once

#include "NativeGameplayTags.h"

/**
 * Central definition of all native gameplay tags for Nexus
 * Keeps everything type-safe and avoids string lookups
 */

namespace NexusGameplayTags
{
	// -------------------------
	// Ability
	// -------------------------
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Enemy_Attack);
	
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Movement_Dash);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Damage_AOE_ArcanePulse);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Buff_Enrage);
	
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Weapon_Axe_Swing);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Weapon_Axe_Deflect);

	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Weapon_Dagger_Slash);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Weapon_Dagger_ShadowStrike);
	
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Weapon_Staff_Fireball);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Weapon_Staff_Repulse);
	

	// -------------------------
	// Cooldown
	// -------------------------

	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Enemy_Attack);
	
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Ability_Dash);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Ability_ArcanePulse);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Ability_Enrage);
	
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Weapon_Staff_Fireball);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Weapon_Staff_Repulse);

	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Weapon_Dagger_Slash);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Weapon_Dagger_ShadowStrike);
	
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Weapon_Axe_Swing);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Weapon_Axe_Deflect);

	// -------------------------
	// Data
	// -------------------------
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Value_Cooldown);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Value_Damage);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Value_Heal);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Value_Stamina);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Value_Duration);

	// -------------------------
	// Event
	// -------------------------
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Ability_Updated);
	
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Ability_Dash_Avtivated);

	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Ability_Repulse_Avtivated);
	
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Ability_ShadowStrike_Impact);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Ability_ShadowStrike_Activated);
	
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Enemy_TargetUpdated);
	
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_HitScan_Start);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_HitScan_End);
	
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Projectile_Explode);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Projectile_Spawn);

	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Deflect_Triggered);

	// -------------------------
	// GameplayCue
	// -------------------------
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Damage_Deflected);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Damage_Burst);
	
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Heal_Burst);
	
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Ability_Dash_Activate);
	
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Ability_Repulse_Activate);
	
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Ability_ShadowStrike_Activate);
	
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Ability_ArcanePulse_Activate);
	
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Projectile_Spawned);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Projectile_Attached);

	// -------------------------
	// Input
	// -------------------------
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Ability_Slot1);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Ability_Slot2);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Ability_Slot3);
	
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Action_Jump);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Action_Move);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Action_Look);
	
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Weapon_PrimaryWeaponAttack);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Weapon_SecondaryWeaponAttack);

	// -------------------------
	// Status
	// -------------------------
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Ability_Active);
	
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Dead);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Stealth);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Stunned);
	
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Defense_Deflecting);
	
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Objective_Capturing);
	
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Stamina_Regen);

	// -------------------------
	// Weapon
	// -------------------------
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Weapon_Melee_Axe);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Weapon_Ranged_Staff);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Weapon_Melee_Daggers);
}
