#include "Nexus/Character/Player/NexusPlayerCharacter.h"

#include "Camera/CameraComponent.h"
#include "GameplayEffect.h"
#include "Components/InputComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Nexus/NexusGameplayTags.h"
#include "Nexus/Components/CharacterClassComponent.h"
#include "Nexus/Components/CombatLoadoutComponent.h"
#include "Nexus/Components/NexusEnhancedInputComponent.h"
#include "Nexus/Components/NexusWeaponsManager.h"
#include "Nexus/Controller/NexusPlayerController.h"
#include "Nexus/DataAssets/InputConfigInfo.h"
#include "Nexus/FunctionLibraries/NexusAbilityFunctionLibrary.h"
#include "Nexus/GameMode/NexusGameMode.h"
#include "Nexus/GameplayAbilitySystem/Abilities/NexusGameplayAbility.h"
#include "Nexus/GameplayAbilitySystem/AbilitySystemComponent/NexusAbilitySystemComponent.h"
#include "Nexus/GameplayAbilitySystem/AttributeSet/PlayerAttributeSet.h"
#include "Nexus/PlayerState/NexusPlayerState.h"
#include "UObject/ConstructorHelpers.h"

ANexusPlayerCharacter::ANexusPlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	WeaponsManager = CreateDefaultSubobject<UNexusWeaponsManager>(TEXT("WeaponsManager"));
	WeaponsManager->SetIsReplicated(true);

	ClassComponent = CreateDefaultSubobject<UCharacterClassComponent>(TEXT("CharacterClassComponent"));
	ClassComponent->SetIsReplicated(true);

	CombatLoadoutComponent = CreateDefaultSubobject<UCombatLoadoutComponent>(TEXT("CombatLoadoutComponent"));
	PlayerAttributeSet = CreateDefaultSubobject<UPlayerAttributeSet>(TEXT("PlayerAttributeSet"));

	NexusEnhancedInputComponent = CreateDefaultSubobject<UNexusEnhancedInputComponent>(TEXT("NexusEnhancedInputComponent"));

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 360.0f;
	CameraBoom->SocketOffset = FVector(0.0f, 60.0f, 70.0f);
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bInheritPitch = true;
	CameraBoom->bInheritYaw = true;
	CameraBoom->bInheritRoll = false;
	CameraBoom->bDoCollisionTest = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	MoveComp->bOrientRotationToMovement = true;
	MoveComp->RotationRate.Yaw = 500.f;

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> DefaultPlayerVisualMeshFinder(
		TEXT("/Game/Mixamo/T-Pose_UE.T-Pose_UE"));
	if (DefaultPlayerVisualMeshFinder.Succeeded())
	{
		DefaultVisualCharacterMesh = DefaultPlayerVisualMeshFinder.Object;
	}

	ApplyDefaultVisualMesh();
}

void ANexusPlayerCharacter::InitializeAbilityActorInfo()
{
	Super::InitializeAbilityActorInfo();
	BindPlayerAttributeDelegates();
	RefreshStaminaRegenState();
}

void ANexusPlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	PitchOffset = FRotator::NormalizeAxis(GetBaseAimRotation().Pitch);
}

void ANexusPlayerCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
}

void ANexusPlayerCharacter::OnRep_Controller()
{
	Super::OnRep_Controller();
}

void ANexusPlayerCharacter::InitializeFromPlayerState()
{
	Super::InitializeFromPlayerState();

	if (CombatLoadoutComponent)
	{
		CombatLoadoutComponent->RefreshFromCurrentPlayerState();
	}
}

void ANexusPlayerCharacter::InitializeCombatLoadout()
{
	if (!HasAuthority())
	{
		return;
	}

	if (CombatLoadoutComponent)
	{
		CombatLoadoutComponent->RefreshFromCurrentPlayerState();
	}
}

void ANexusPlayerCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	ApplyDefaultVisualMesh();
}

void ANexusPlayerCharacter::ApplyDeathState_Server()
{
	Super::ApplyDeathState_Server();
	RefreshStaminaRegenState();

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

void ANexusPlayerCharacter::ApplyDefaultVisualMesh()
{
	USkeletalMeshComponent* MeshComp = GetMesh();
	if (!MeshComp || !DefaultVisualCharacterMesh)
	{
		return;
	}

	if (ClassComponent && ClassComponent->HasAppliedClass())
	{
		return;
	}

	if (MeshComp->GetSkeletalMeshAsset() != DefaultVisualCharacterMesh)
	{
		MeshComp->SetSkeletalMeshAsset(DefaultVisualCharacterMesh);
	}
}

void ANexusPlayerCharacter::BindPlayerAttributeDelegates()
{
	if (!AbilitySystemComponent || !PlayerAttributeSet)
	{
		return;
	}

	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
		PlayerAttributeSet->GetStaminaAttribute()).RemoveAll(this);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
		PlayerAttributeSet->GetMaxStaminaAttribute()).RemoveAll(this);

	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
		PlayerAttributeSet->GetStaminaAttribute()).AddUObject(this, &ThisClass::HandleStaminaAttributeChanged);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
		PlayerAttributeSet->GetMaxStaminaAttribute()).AddUObject(this, &ThisClass::HandleMaxStaminaAttributeChanged);
}

void ANexusPlayerCharacter::RefreshStaminaRegenState()
{
	if (!HasAuthority() || !AbilitySystemComponent || !PlayerAttributeSet)
	{
		return;
	}

	FGameplayTagContainer RegenGrantedTags;
	RegenGrantedTags.AddTag(NexusGameplayTags::Status_Stamina_Regen);

	const bool bWantsStaminaRegen =
		!bIsDead &&
		StaminaRegenEffect &&
		PlayerAttributeSet->GetStamina() < PlayerAttributeSet->GetMaxStamina();
	const bool bHasStaminaRegen =
		AbilitySystemComponent->HasMatchingGameplayTag(NexusGameplayTags::Status_Stamina_Regen);

	if (!bWantsStaminaRegen)
	{
		if (bHasStaminaRegen)
		{
			AbilitySystemComponent->RemoveActiveEffectsWithGrantedTags(RegenGrantedTags);
		}

		return;
	}

	if (bHasStaminaRegen)
	{
		return;
	}

	const UGameplayEffect* StaminaRegenEffectCDO = StaminaRegenEffect->GetDefaultObject<UGameplayEffect>();
	if (!StaminaRegenEffectCDO)
	{
		return;
	}

	FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
	EffectContext.AddInstigator(this, this);
	EffectContext.AddSourceObject(this);

	AbilitySystemComponent->ApplyGameplayEffectToSelf(
		StaminaRegenEffectCDO,
		1.0f,
		EffectContext);
}

void ANexusPlayerCharacter::HandleStaminaAttributeChanged(const FOnAttributeChangeData& ChangeData)
{
	RefreshStaminaRegenState();
}

void ANexusPlayerCharacter::HandleMaxStaminaAttributeChanged(const FOnAttributeChangeData& ChangeData)
{
	RefreshStaminaRegenState();
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

	ANexusCharacterBase* ResolvedTarget = PC->GetUsableTargetForAbility(AbilityCDO);
	if (!ResolvedTarget)
	{
		return false;
	}

	OutTargetCharacter = ResolvedTarget;
	return true;
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
