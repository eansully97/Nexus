#include "Nexus/DataAssets/CharacterClassInfo.h"

#include "Nexus/Weapons/NexusWeaponBase.h"

bool UCharacterClassInfo::IsWeaponAllowed(TSubclassOf<ANexusWeaponBase> WeaponClass) const
{
	if (!WeaponClass)
	{
		return false;
	}

	if (AllowedWeaponClasses.IsEmpty())
	{
		return DefaultWeaponClass == WeaponClass;
	}

	for (const TSubclassOf<ANexusWeaponBase>& AllowedClass : AllowedWeaponClasses)
	{
		if (AllowedClass == WeaponClass)
		{
			return true;
		}
	}

	return false;
}

TSubclassOf<ANexusWeaponBase> UCharacterClassInfo::GetResolvedDefaultWeaponClass() const
{
	if (DefaultWeaponClass)
	{
		return DefaultWeaponClass;
	}

	if (!AllowedWeaponClasses.IsEmpty())
	{
		return AllowedWeaponClasses[0];
	}

	return nullptr;
}