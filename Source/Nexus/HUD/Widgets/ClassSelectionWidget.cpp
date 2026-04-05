#include "Nexus/HUD/Widgets/ClassSelectionWidget.h"

#include "Nexus/Controller/NexusPlayerController.h"
#include "Nexus/Weapons/NexusWeaponBase.h"

ANexusPlayerController* UClassSelectionWidget::GetNexusPC() const
{
	return GetOwningPlayer<ANexusPlayerController>();
}

void UClassSelectionWidget::SelectClass(UCharacterClassInfo* InClassInfo)
{
	PendingClass = InClassInfo;
	PendingWeapon = nullptr;
	PendingClassAbilities.Reset();

	if (ANexusPlayerController* PC = GetNexusPC())
	{
		PC->Server_SelectClass(InClassInfo);
	}
}

void UClassSelectionWidget::SelectWeapon(TSubclassOf<ANexusWeaponBase> InWeaponClass)
{
	PendingWeapon = InWeaponClass;

	if (ANexusPlayerController* PC = GetNexusPC())
	{
		PC->Server_SelectWeapon(InWeaponClass);
	}
}

void UClassSelectionWidget::SetSelectedClassAbilities(const TArray<FNexusAbilityGrant>& InAbilityGrants)
{
	PendingClassAbilities = InAbilityGrants;

	if (ANexusPlayerController* PC = GetNexusPC())
	{
		PC->Server_SetSelectedClassAbilities(InAbilityGrants);
	}
}

void UClassSelectionWidget::ClickReady()
{
	if (ANexusPlayerController* PC = GetNexusPC())
	{
		PC->Server_SetReady(true);
	}
}

void UClassSelectionWidget::ClickUnready()
{
	if (ANexusPlayerController* PC = GetNexusPC())
	{
		PC->Server_SetReady(false);
	}
}