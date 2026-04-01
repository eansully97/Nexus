// NexusInputConfig.cpp

#include "InputConfigInfo.h"


const UInputAction* UInputConfigInfo::FindNativeInputActionForTag(const FGameplayTag& InputTag, bool bLogNotFound) const
{
	for (const FNexusInputAction& Action : NativeInputActions)
	{
		if (Action.InputTag == InputTag && Action.InputAction)
		{
			return Action.InputAction;
		}
	}

	if (bLogNotFound)
	{
		UE_LOG(LogTemp, Warning, TEXT("Native InputAction not found for tag [%s]"), *InputTag.ToString());
	}

	return nullptr;
}

const UInputAction* UInputConfigInfo::FindAbilityInputActionForTag(const FGameplayTag& InputTag, bool bLogNotFound) const
{
	for (const FNexusInputAction& Action : AbilityInputActions)
	{
		if (Action.InputTag == InputTag && Action.InputAction)
		{
			return Action.InputAction;
		}
	}

	if (bLogNotFound)
	{
		UE_LOG(LogTemp, Warning, TEXT("Ability InputAction not found for tag [%s]"), *InputTag.ToString());
	}

	return nullptr;
}