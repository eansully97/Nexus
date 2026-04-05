#include "Nexus/Character/Player/NexusPlayerCharacter.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "Nexus/NexusGameplayTags.h"
#include "Nexus/Components/CharacterClassComponent.h"
#include "Nexus/Components/NexusEnhancedInputComponent.h"
#include "Nexus/Components/NexusWeaponsManager.h"
#include "Nexus/Controller/NexusPlayerController.h"
#include "Nexus/FunctionLibraries/NexusAbilityFunctionLibrary.h"
#include "Nexus/GameMode/NexusGameMode.h"
#include "Nexus/GameplayAbilitySystem/Abilities/NexusGameplayAbility.h"
#include "Nexus/GameplayAbilitySystem/AbilitySystemComponent/NexusAbilitySystemComponent.h"
#include "Nexus/PlayerState/NexusPlayerState.h"
#include "Nexus/Weapons/NexusWeaponBase.h"

ANexusPlayerCharacter::ANexusPlayerCharacter()
{
	WeaponsManager = CreateDefaultSubobject<UNexusWeaponsManager>(TEXT("WeaponsManager"));
	WeaponsManager->SetIsReplicated(true);

	ClassComponent = CreateDefaultSubobject<UCharacterClassComponent>(TEXT("CharacterClassComponent"));
	ClassComponent->SetIsReplicated(true);

	NexusEnhancedInputComponent = CreateDefaultSubobject<UNexusEnhancedInputComponent>(TEXT("NexusEnhancedInputComponent"));

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
	if (!PS)
	{
		return;
	}

	if (ClassComponent)
	{
		ClassComponent->ApplyClassFromPlayerState(PS);
	}

	if (HasAuthority())
	{
		PS->ApplyPersistentCombatProfileToCharacter(this);
	}
}

void ANexusPlayerCharacter::InitializeCombatLoadout()
{
	if (!HasAuthority())
	{
		return;
	}

	RebuildCombatLoadoutPlayerOnly();
}

void ANexusPlayerCharacter::RebuildCombatLoadoutPlayerOnly()
{
	ClearAbilitySet(ENexusAbilitySource::Class);

	if (WeaponsManager)
	{
		WeaponsManager->UnequipCurrentWeapon();
	}

	const TArray<FNexusAbilityGrant> ClassGrants = GetClassAbilitiesToGrant();
	if (ClassGrants.Num() > 0)
	{
		GrantAbilitySet(ENexusAbilitySource::Class, ClassGrants, this);
	}

	const ANexusPlayerState* PS = GetPlayerState<ANexusPlayerState>();
	if (PS && WeaponsManager && PS->GetSelectedWeaponClass())
	{
		WeaponsManager->EquipOrSwap(PS->GetSelectedWeaponClass());
	}

	OnCombatStateChanged.Broadcast();
}

TArray<FNexusAbilityGrant> ANexusPlayerCharacter::GetClassAbilitiesToGrant() const
{
	const ANexusPlayerState* PS = GetPlayerState<ANexusPlayerState>();
	return PS ? PS->GetClassAbilityGrants() : TArray<FNexusAbilityGrant>();
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

void ANexusPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (PC)
	{
		if (ULocalPlayer* LocalPlayer = PC->GetLocalPlayer())
		{
			if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
				LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
			{
				if (DefaultMappingContext)
				{
					Subsystem->AddMappingContext(DefaultMappingContext, 0);
				}
			}
		}
	}

	UNexusEnhancedInputComponent* NexusInput = CastChecked<UNexusEnhancedInputComponent>(PlayerInputComponent);
	check(InputConfig);

	NexusInput->BindNativeAction(InputConfig, NexusGameplayTags::Input_Action_Move, ETriggerEvent::Triggered, this, &ThisClass::Input_Move);
	NexusInput->BindNativeAction(InputConfig, NexusGameplayTags::Input_Action_Look, ETriggerEvent::Triggered, this, &ThisClass::Input_Look);
	NexusInput->BindNativeAction(InputConfig, NexusGameplayTags::Input_Action_Jump, ETriggerEvent::Started, this, &ThisClass::Input_JumpPressed);
	NexusInput->BindNativeAction(InputConfig, NexusGameplayTags::Input_Action_Jump, ETriggerEvent::Completed, this, &ThisClass::Input_JumpReleased);

	NexusInput->BindAbilityActions(InputConfig, this, &ThisClass::Input_AbilityPressed, &ThisClass::Input_AbilityReleased);
}

void ANexusPlayerCharacter::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ThisClass, PitchOffset);
}

bool ANexusPlayerCharacter::ShouldBlockNativeInput() const
{
	return AbilitySystemComponent &&
		AbilitySystemComponent->HasMatchingGameplayTag(NexusGameplayTags::Status_Ability_Active);
}

bool ANexusPlayerCharacter::CanAcceptGameplayInput() const
{
	if (bIsDead)
	{
		return false;
	}

	if (!AbilitySystemComponent)
	{
		return true;
	}

	if (AbilitySystemComponent->HasMatchingGameplayTag(NexusGameplayTags::Status_Stunned) ||
		AbilitySystemComponent->HasMatchingGameplayTag(NexusGameplayTags::Status_Dead))
	{
		return false;
	}

	return true;
}

void ANexusPlayerCharacter::Input_AbilityPressed(FGameplayTag InputTag)
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	if (TrySendAbilityGameplayEvent(InputTag))
	{
		return;
	}

	AbilitySystemComponent->AbilityInputTagPressed(InputTag);
}

void ANexusPlayerCharacter::Input_AbilityReleased(FGameplayTag InputTag)
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->AbilityInputTagReleased(InputTag);
	}
}

bool ANexusPlayerCharacter::TryResolveUsableTargetForAbility(
	const UNexusGameplayAbility* AbilityCDO,
	ANexusCharacterBase*& OutTargetCharacter) const
{
	OutTargetCharacter = nullptr;

	if (!AbilityCDO)
	{
		return false;
	}

	const ANexusPlayerController* PC = Cast<ANexusPlayerController>(GetController());
	if (!PC)
	{
		return false;
	}

	return PC->GetUsableTargetForAbility(AbilityCDO) != nullptr
		? (OutTargetCharacter = PC->GetUsableTargetForAbility(AbilityCDO), true)
		: false;
}

bool ANexusPlayerCharacter::TrySendAbilityGameplayEvent(FGameplayTag InputTag)
{
	const FGameplayAbilitySpec* Spec = FindAbilitySpecByInputTag(InputTag);
	const UNexusGameplayAbility* AbilityCDO = Spec ? Cast<UNexusGameplayAbility>(Spec->Ability) : nullptr;

	if (!AbilityCDO || !AbilityCDO->bActivateByEvent)
	{
		return false;
	}

	const FGameplayTag EventTag = AbilityCDO->GetPrimaryActivationEventTag();
	if (!EventTag.IsValid())
	{
		return false;
	}

	AActor* TargetActor = nullptr;

	if (AbilityCDO->AbilityRequiresUsableTarget())
	{
		ANexusCharacterBase* TargetCharacter = nullptr;
		if (!TryResolveUsableTargetForAbility(AbilityCDO, TargetCharacter))
		{
			return true;
		}

		TargetActor = TargetCharacter;
		Server_SendAbilityTargetedEvent(InputTag, TargetActor);
		return true;
	}

	UNexusAbilityFunctionLibrary::SendTargetedGameplayEventToActor(
		this,
		EventTag,
		nullptr,
		nullptr);

	return true;
}

void ANexusPlayerCharacter::Server_SendAbilityTargetedEvent_Implementation(FGameplayTag InputTag, AActor* TargetActor)
{
	const FGameplayAbilitySpec* Spec = FindAbilitySpecByInputTag(InputTag);
	const UNexusGameplayAbility* AbilityCDO = Spec ? Cast<UNexusGameplayAbility>(Spec->Ability) : nullptr;
	if (!AbilityCDO)
	{
		return;
	}

	const FGameplayTag EventTag = AbilityCDO->GetPrimaryActivationEventTag();
	if (!EventTag.IsValid())
	{
		return;
	}

	ANexusCharacterBase* TargetCharacter = Cast<ANexusCharacterBase>(TargetActor);
	if (!UNexusAbilityFunctionLibrary::IsAbilityTargetUsable(AbilityCDO, this, TargetCharacter))
	{
		if (AbilityCDO->bRequiresValidTarget)
		{
			return;
		}

		TargetCharacter = nullptr;
		TargetActor = nullptr;
	}

	UNexusAbilityFunctionLibrary::SendTargetedGameplayEventToActor(
		this,
		EventTag,
		TargetActor,
		TargetActor);
}

const FGameplayAbilitySpec* ANexusPlayerCharacter::FindAbilitySpecByInputTag(FGameplayTag InputTag) const
{
	const UNexusAbilitySystemComponent* NexusASC = Cast<UNexusAbilitySystemComponent>(AbilitySystemComponent);
	return NexusASC ? NexusASC->FindAbilitySpecByInputTag(InputTag) : nullptr;
}

UCharacterClassInfo* ANexusPlayerCharacter::GetClassInfo()
{
	ANexusPlayerState* PS = GetPlayerState<ANexusPlayerState>();
	return PS ? PS->GetCharacterClassInfo() : nullptr;
}

void ANexusPlayerCharacter::Input_Move(const FInputActionValue& Value)
{
	if (ShouldBlockNativeInput())
	{
		return;
	}

	const FVector2D InputAxis = Value.Get<FVector2D>();
	if (!Controller)
	{
		return;
	}

	const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
	const FVector Forward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector Right = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	AddMovementInput(Forward, InputAxis.Y);
	AddMovementInput(Right, InputAxis.X);
}

void ANexusPlayerCharacter::Input_Look(const FInputActionValue& Value)
{
	if (!Controller)
	{
		return;
	}

	const FVector2D LookAxis = Value.Get<FVector2D>();
	const ANexusPlayerController* NexusPC = Cast<ANexusPlayerController>(Controller);
	const float AppliedSensitivity = NexusPC ? NexusPC->LookSensitivity : 1.f;

	AddControllerYawInput(LookAxis.X * AppliedSensitivity);
	AddControllerPitchInput(LookAxis.Y * AppliedSensitivity);

	const float Pitch = GetControlRotation().Pitch;
	Server_SetPitch(Pitch);
}

void ANexusPlayerCharacter::Server_SetPitch_Implementation(float InPitch)
{
	Multicast_SetPitch(InPitch);
}

void ANexusPlayerCharacter::Multicast_SetPitch_Implementation(float InPitch)
{
	PitchOffset = InPitch;
}

void ANexusPlayerCharacter::Input_JumpPressed(const FInputActionValue& Value)
{
	if (ShouldBlockNativeInput())
	{
		return;
	}

	Jump();
}

void ANexusPlayerCharacter::Input_JumpReleased(const FInputActionValue& Value)
{
	StopJumping();
}