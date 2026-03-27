// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerStart.h"
#include "Nexus/NexusEnumTypes.h"
#include "NexusPlayerStart.generated.h"

UCLASS()
class NEXUS_API ANexusPlayerStart : public APlayerStart
{
	GENERATED_BODY()

public:
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Team")
	ENexusTeamID TeamID = ENexusTeamID::Neutral;
};
