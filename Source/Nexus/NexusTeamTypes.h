#pragma once

#include "NexusTeamTypes.generated.h"

UENUM(BlueprintType)
enum class ENexusTeamID : uint8
{
	Neutral UMETA(DisplayName="Neutral"),
	TeamA   UMETA(DisplayName="Team A"),
	TeamB   UMETA(DisplayName="Team B")
};