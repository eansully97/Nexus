#include "NexusGameplayTags.h"

namespace NexusGameplayTags
{
	// -------------------------
	// Ability
	// -------------------------
	UE_DEFINE_GAMEPLAY_TAG(Ability_Reactive_Parry, "Ability.Reactive.Parry");
	
	UE_DEFINE_GAMEPLAY_TAG(Ability_Enemy_Attack, "Ability.Enemy.Attack");
	
	UE_DEFINE_GAMEPLAY_TAG(Ability_Movement_Dash, "Ability.Movement.Dash");
	
	UE_DEFINE_GAMEPLAY_TAG(Ability_Weapon_Axe_Swing, "Ability.Weapon.Axe.Swing");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Weapon_Axe_Block, "Ability.Weapon.Axe.Block");
	
	UE_DEFINE_GAMEPLAY_TAG(Ability_Weapon_Dagger_Slash, "Ability.Weapon.Dagger.Slash");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Weapon_Dagger_ShadowStrike, "Ability.Weapon.Dagger.ShadowStrike");
	
	UE_DEFINE_GAMEPLAY_TAG(Ability_Weapon_Staff_Fireball, "Ability.Weapon.Staff.Fireball");
	UE_DEFINE_GAMEPLAY_TAG(Ability_Weapon_Staff_ArcanePulse, "Ability.Weapon.Staff.ArcanePulse");

	// -------------------------
	// Cooldown
	// -------------------------
	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Enemy_Attack, "Cooldown.Enemy.Attack");
	
	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Ability_Dash, "Cooldown.Ability.Dash");
	
	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Weapon_Axe_Swing, "Cooldown.Weapon.Axe.Swing");
	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Weapon_Axe_Block, "Cooldown.Weapon.Axe.Block");

	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Weapon_Dagger_Slash, "Cooldown.Weapon.Dagger.Slash");
	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Weapon_Dagger_ShadowStrike, "Cooldown.Weapon.Dagger.ShadowStrike");
	
	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Weapon_Staff_Fireball, "Cooldown.Weapon.Staff.Fireball");
	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Weapon_Staff_ArcanePulse, "Cooldown.Weapon.Staff.ArcanePulse");

	// -------------------------
	// Data
	// -------------------------
	UE_DEFINE_GAMEPLAY_TAG(Data_Value_Cooldown, "Data.Value.Cooldown");
	UE_DEFINE_GAMEPLAY_TAG(Data_Value_Damage, "Data.Value.Damage");
	UE_DEFINE_GAMEPLAY_TAG(Data_Value_Heal, "Data.Value.Heal");
	UE_DEFINE_GAMEPLAY_TAG(Data_Value_Duration, "Data.Value.Duration");

	// -------------------------
	// Event
	// -------------------------
	UE_DEFINE_GAMEPLAY_TAG(Event_Ability_Updated, "Event.Ability.Updated");
	
	UE_DEFINE_GAMEPLAY_TAG(Event_Ability_Dash_Avtivated, "Event.Ability.Dash.Activated");
	UE_DEFINE_GAMEPLAY_TAG(Event_Ability_Parry_Activated, "Event.Ability.Parry.Activated");
	
	UE_DEFINE_GAMEPLAY_TAG(Event_Ability_ShadowStrike_Impact, "Event.Ability.ShadowStrike.Impact");
	UE_DEFINE_GAMEPLAY_TAG(Event_Ability_ShadowStrike_Activated, "Event.Ability.ShadowStrike.Activated");
	
	UE_DEFINE_GAMEPLAY_TAG(Event_Enemy_TargetUpdated, "Event.Enemy.TargetUpdated");
	
	UE_DEFINE_GAMEPLAY_TAG(Event_HitScan_Start, "Event.HitScan.Start");
	UE_DEFINE_GAMEPLAY_TAG(Event_HitScan_End, "Event.HitScan.End");
	
	UE_DEFINE_GAMEPLAY_TAG(Event_Projectile_Explode, "Event.Projectile.Explode");
	UE_DEFINE_GAMEPLAY_TAG(Event_Projectile_Spawn, "Event.Projectile.Spawn");

	// -------------------------
	// GameplayCue
	// -------------------------
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Damage_Blocked, "GameplayCue.Damage.Blocked");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Damage_Burst, "GameplayCue.Damage.Burst");
	
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Heal_Burst, "GameplayCue.Heal.Burst");
	
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Ability_Dash_Activate, "GameplayCue.Ability.Dash.Activate");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Ability_ArcanePulse_Activate, "GameplayCue.Ability.ArcanePulse.Activate");

	// -------------------------
	// Input
	// -------------------------
	UE_DEFINE_GAMEPLAY_TAG(Input_Ability_Dash, "Input.Ability.Dash");
	
	UE_DEFINE_GAMEPLAY_TAG(Input_Action_Jump, "Input.Action.Jump");
	UE_DEFINE_GAMEPLAY_TAG(Input_Action_Move, "Input.Action.Move");
	UE_DEFINE_GAMEPLAY_TAG(Input_Action_Look, "Input.Action.Look");
	
	UE_DEFINE_GAMEPLAY_TAG(Input_Weapon_PrimaryWeaponAttack, "Input.Weapon.PrimaryWeaponAttack");
	UE_DEFINE_GAMEPLAY_TAG(Input_Weapon_SecondaryWeaponAttack, "Input.Weapon.SecondaryWeaponAttack");

	// -------------------------
	// Status
	// -------------------------
	UE_DEFINE_GAMEPLAY_TAG(Status_Ability_Active, "Status.Ability.Active");
	
	UE_DEFINE_GAMEPLAY_TAG(Status_Dead, "Status.Dead");
	UE_DEFINE_GAMEPLAY_TAG(Status_Stealth, "Status.Stealth");
	UE_DEFINE_GAMEPLAY_TAG(Status_Stunned, "Status.Stunned");
	
	UE_DEFINE_GAMEPLAY_TAG(Status_Defense_Blocking, "Status.Defense.Blocking");
	
	UE_DEFINE_GAMEPLAY_TAG(Status_Objective_Capturing, "Status.Objective.Capturing");
	
	UE_DEFINE_GAMEPLAY_TAG(Status_Stamina_Regen, "Status.Stamina.Regen");

	// -------------------------
	// Weapon
	// -------------------------
	UE_DEFINE_GAMEPLAY_TAG(Weapon_Melee_Axe, "Weapon.Melee.Axe");
	UE_DEFINE_GAMEPLAY_TAG(Weapon_Ranged_Staff, "Weapon.Ranged.Staff");
	UE_DEFINE_GAMEPLAY_TAG(Weapon_Melee_Daggers, "Weapon.Melee.Daggers");
}