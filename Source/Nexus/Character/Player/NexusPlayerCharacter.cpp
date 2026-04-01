#include "NexusPlayerCharacter.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "EnhancedInputSubsystems.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "Nexus/NexusGameplayTags.h"
#include "Nexus/Components/CharacterClassComponent.h"
#include "Nexus/Components/NexusEnhancedInputComponent.h"
#include "Nexus/Components/NexusWeaponsManager.h"
#include "Nexus/Controller/NexusPlayerController.h"
#include "Nexus/GameMode/NexusGameMode.h"
#include "Nexus/GameplayAbilitySystem/Abilities/NexusGameplayAbility.h"
#include "Nexus/GameplayAbilitySystem/AbilitySystemComponent/NexusAbilitySystemComponent.h"
#include "Nexus/PlayerState/NexusPlayerState.h"


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
	if (!PS) return;

	PS->ApplyPersistentCombatProfileToCharacter(this);

	if (HasAuthority() && WeaponsManager && PS->GetCharacterClassInfo())
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
	DOREPLIFETIME(ThisClass, PitchOffset)
}

bool ANexusCharacterBase::ShouldBlockNativeInput() const
{
	return AbilitySystemComponent &&
		AbilitySystemComponent->HasMatchingGameplayTag(NexusGameplayTags::Status_Ability_Active);
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

bool ANexusPlayerCharacter::TrySendAbilityGameplayEvent(FGameplayTag InputTag)
{
	const FGameplayAbilitySpec* Spec = FindAbilitySpecByInputTag(InputTag);
	if (!Spec)
	{
		return false;
	}

	const UNexusGameplayAbility* AbilityCDO = Cast<UNexusGameplayAbility>(Spec->Ability);
	if (!AbilityCDO)
	{
		return false;
	}

	if (!AbilityCDO->bActivateByEvent)
	{
		return false;
	}

	AActor* TargetActor = nullptr;

	if (AbilityCDO->bRequiresValidTarget)
	{
		ANexusPlayerController* PC = Cast<ANexusPlayerController>(GetController());
		if (!IsValid(PC) || !PC->HasValidTarget())
		{
			return true;
		}

		TargetActor = PC->GetCurrentTargetedCharacter();
	}

	Server_SendAbilityTargetedEvent(InputTag, TargetActor);
	return true;
}

void ANexusPlayerCharacter::Server_SendAbilityTargetedEvent_Implementation(FGameplayTag InputTag, AActor* TargetActor)
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	const FGameplayAbilitySpec* Spec = FindAbilitySpecByInputTag(InputTag);
	if (!Spec)
	{
		return;
	}

	const UNexusGameplayAbility* AbilityCDO = Cast<UNexusGameplayAbility>(Spec->Ability);
	if (!AbilityCDO || !AbilityCDO->bActivateByEvent)
	{
		return;
	}

	FGameplayEventData Payload;
	Payload.Instigator = this;
	Payload.Target = TargetActor;
	Payload.EventMagnitude = AbilityCDO->Damage * -1;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
		this,
		AbilityCDO->AbilityTagConfig.ActivationEventTag,
		Payload);
}

const FGameplayAbilitySpec* ANexusPlayerCharacter::FindAbilitySpecByInputTag(FGameplayTag InputTag) const
{
	if (!AbilitySystemComponent || !InputTag.IsValid())
	{
		return nullptr;
	}

	for (const FGameplayAbilitySpec& Spec : AbilitySystemComponent->GetActivatableAbilities())
	{
		if (Spec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
		{
			return &Spec;
		}
	}

	return nullptr;
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
	if (!Controller) return;

	const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
	const FVector Forward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector Right = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	AddMovementInput(Forward, InputAxis.Y);
	AddMovementInput(Right, InputAxis.X);
}

void ANexusPlayerCharacter::Input_Look(const FInputActionValue& Value)
{
	const FVector2D LookAxis = Value.Get<FVector2D>();

	AddControllerYawInput(LookAxis.X);
	AddControllerPitchInput(LookAxis.Y);
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
