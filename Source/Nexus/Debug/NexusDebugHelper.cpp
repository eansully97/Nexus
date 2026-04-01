#include "NexusDebugHelper.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "NexusLog.h"

FString UNexusDebugHelper::GetNetModeString(const UObject* SourceObject)
{
	if (!SourceObject)
	{
		return TEXT("NoObject");
	}

	const UWorld* World = SourceObject->GetWorld();
	if (!World)
	{
		return TEXT("NoWorld");
	}

	switch (World->GetNetMode())
	{
	case NM_Standalone:     return TEXT("Standalone");
	case NM_DedicatedServer:return TEXT("DedicatedServer");
	case NM_ListenServer:   return TEXT("ListenServer");
	case NM_Client:         return TEXT("Client");
	default:                return TEXT("UnknownNetMode");
	}
}

FString UNexusDebugHelper::GetLocalRoleString(const AActor* Actor)
{
	if (!Actor)
	{
		return TEXT("NoActor");
	}

	switch (Actor->GetLocalRole())
	{
	case ROLE_None:              return TEXT("None");
	case ROLE_SimulatedProxy:    return TEXT("SimulatedProxy");
	case ROLE_AutonomousProxy:   return TEXT("AutonomousProxy");
	case ROLE_Authority:         return TEXT("Authority");
	default:                     return TEXT("UnknownLocalRole");
	}
}

FString UNexusDebugHelper::GetRemoteRoleString(const AActor* Actor)
{
	if (!Actor)
	{
		return TEXT("NoActor");
	}

	switch (Actor->GetRemoteRole())
	{
	case ROLE_None:              return TEXT("None");
	case ROLE_SimulatedProxy:    return TEXT("SimulatedProxy");
	case ROLE_AutonomousProxy:   return TEXT("AutonomousProxy");
	case ROLE_Authority:         return TEXT("Authority");
	default:                     return TEXT("UnknownRemoteRole");
	}
}

FString UNexusDebugHelper::BuildNetContextString(const UObject* SourceObject)
{
	if (!SourceObject)
	{
		return TEXT("[NoSource]");
	}

	const AActor* Actor = Cast<AActor>(SourceObject);
	const APawn* Pawn = Cast<APawn>(SourceObject);

	FString ObjectName = SourceObject->GetName();
	FString NetMode = GetNetModeString(SourceObject);

	FString RoleInfo = TEXT("");
	if (Actor)
	{
		RoleInfo = FString::Printf(
			TEXT(" LocalRole=%s RemoteRole=%s"),
			*GetLocalRoleString(Actor),
			*GetRemoteRoleString(Actor)
		);
	}

	FString LocalControlInfo = TEXT("");
	if (Pawn)
	{
		LocalControlInfo = Pawn->IsLocallyControlled() ? TEXT(" LocallyControlled=true") : TEXT(" LocallyControlled=false");
	}

	return FString::Printf(TEXT("[%s | %s%s%s]"), *ObjectName, *NetMode, *RoleInfo, *LocalControlInfo);
}

void UNexusDebugHelper::DebugMessage(
	const UObject* WorldContextObject,
	const UObject* SourceObject,
	const FString& Message,
	FColor Color,
	float Duration,
	bool bPrintToScreen,
	bool bPrintToLog)
{
	const FString Context = BuildNetContextString(SourceObject ? SourceObject : WorldContextObject);
	const FString FinalMessage = FString::Printf(TEXT("%s %s"), *Context, *Message);

	if (bPrintToLog)
	{
		UE_LOG(LogNexus, Warning, TEXT("%s"), *FinalMessage);
	}

	if (bPrintToScreen && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			Duration,
			Color,
			FinalMessage
		);
	}
}