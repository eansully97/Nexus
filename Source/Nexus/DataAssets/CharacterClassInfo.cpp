#include "Nexus/DataAssets/CharacterClassInfo.h"

#include "Engine/SkeletalMesh.h"
#include "Nexus/Weapons/NexusWeaponBase.h"

namespace
{
USkeletalMesh* GetNativeDefaultCharacterMesh(ECharacterClassName CharacterClassName)
{
	switch (CharacterClassName)
	{
	case ECharacterClassName::Warrior:
	{
		static USkeletalMesh* WarriorMesh = LoadObject<USkeletalMesh>(
			nullptr,
			TEXT("/Game/Mixamo/SM_Warrior.SM_Warrior"));
		return WarriorMesh;
	}
	case ECharacterClassName::Mage:
	{
		static USkeletalMesh* MageMesh = LoadObject<USkeletalMesh>(
			nullptr,
			TEXT("/Game/Mixamo/SM_Mage.SM_Mage"));
		return MageMesh;
	}
	case ECharacterClassName::Rogue:
	{
		static USkeletalMesh* RogueMesh = LoadObject<USkeletalMesh>(
			nullptr,
			TEXT("/Game/Mixamo/SM_Rouge.SM_Rouge"));
		return RogueMesh;
	}
	default:
		return nullptr;
	}
}
}

USkeletalMesh* UCharacterClassInfo::GetResolvedCharacterMesh() const
{
	if (CharacterData.CharacterMesh)
	{
		return CharacterData.CharacterMesh;
	}

	if (USkeletalMesh* NativeDefaultMesh = GetNativeDefaultCharacterMesh(CharacterClassName))
	{
		return NativeDefaultMesh;
	}

	return nullptr;
}

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
