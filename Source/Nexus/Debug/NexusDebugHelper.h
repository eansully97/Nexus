#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "NexusDebugHelper.generated.h"

UCLASS()
class NEXUS_API UNexusDebugHelper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Nexus|Debug")
	static void DebugMessage(
		const UObject* WorldContextObject,
		const UObject* SourceObject,
		const FString& Message,
		FColor Color = FColor::Green,
		float Duration = 2.0f,
		bool bPrintToScreen = true,
		bool bPrintToLog = true
	);

	UFUNCTION(BlueprintPure, Category="Nexus|Debug")
	static FString BuildNetContextString(const UObject* SourceObject);

	UFUNCTION(BlueprintPure, Category="Nexus|Debug")
	static FString GetNetModeString(const UObject* SourceObject);

	UFUNCTION(BlueprintPure, Category="Nexus|Debug")
	static FString GetLocalRoleString(const AActor* Actor);

	UFUNCTION(BlueprintPure, Category="Nexus|Debug")
	static FString GetRemoteRoleString(const AActor* Actor);
};