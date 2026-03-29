#include "NexusPlayerCharacter.h"

#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Nexus/Components/CharacterClassComponent.h"
#include "Nexus/Components/NexusWeaponsManager.h"
#include "Nexus/GameMode/NexusGameMode.h"
#include "Nexus/PlayerState/NexusPlayerState.h"
#include "Nexus/Weapons/NexusWeaponBase.h"

ANexusPlayerCharacter::ANexusPlayerCharacter()
{
	WeaponsManager = CreateDefaultSubobject<UNexusWeaponsManager>(TEXT("WeaponsManager"));
	WeaponsManager->SetIsReplicated(true);

	ClassComponent = CreateDefaultSubobject<UCharacterClassComponent>(TEXT("CharacterClassComponent"));
	ClassComponent->SetIsReplicated(true);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	MoveComp->bOrientRotationToMovement = true;
	MoveComp->RotationRate.Yaw = 500.f;
}

void ANexusPlayerCharacter::InitializeFromPlayerState()
{
	Super::InitializeFromPlayerState();

	ANexusPlayerState* PS = GetPlayerState<ANexusPlayerState>();
	if (!PS) return;

	PS->ApplyPersistentCombatProfileToCharacter(this);

	if (WeaponsManager && PS->GetCharacterClassInfo())
	{
		WeaponsManager->Equip(
			PS->GetCharacterClassInfo()->WeaponClassToEquip
		);
	}
}

void ANexusPlayerCharacter::ApplyTeamVisuals() const
{
	Super::ApplyTeamVisuals();

	USkeletalMeshComponent* MeshComp = GetMesh();
	if (!MeshComp)
	{
		return;
	}

	UMaterialInstanceDynamic* MID0 = MeshComp->CreateDynamicMaterialInstance(0);
	UMaterialInstanceDynamic* MID1 = MeshComp->CreateDynamicMaterialInstance(1);

	if (!MID0 || !MID1)
	{
		return;
	}

	switch (TeamID)
	{
	case ENexusTeamID::TeamA:
		MID0->SetVectorParameterValue(TEXT("Paint Tint"), TeamAColor1);
		MID1->SetVectorParameterValue(TEXT("Paint Tint"), TeamAColor2);
		break;

	case ENexusTeamID::TeamB:
		MID0->SetVectorParameterValue(TEXT("Paint Tint"), TeamBColor1);
		MID1->SetVectorParameterValue(TEXT("Paint Tint"), TeamBColor2);
		break;

	default:
		break;
	}
}

void ANexusPlayerCharacter::ApplyDeathState_Server()
{
	Super::ApplyDeathState_Server();

	if (ANexusPlayerState* PS = GetPlayerState<ANexusPlayerState>())
	{
		PS->CapturePersistentCombatStateFromCharacter(this);
	}

	if (GetController() && GetController()->IsPlayerController())
	{
		if (ANexusGameMode* GM = GetWorld()->GetAuthGameMode<ANexusGameMode>())
		{
			GM->RequestRespawn(GetController(), RespawnTime);
		}
	}
}

UCharacterClassInfo* ANexusPlayerCharacter::GetClassInfo()
{
	ANexusPlayerState* PS = GetPlayerState<ANexusPlayerState>();
	return PS ? PS->GetCharacterClassInfo() : nullptr;
}
