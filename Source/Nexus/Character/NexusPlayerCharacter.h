// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NexusCharacterBase.h"
#include "NexusPlayerCharacter.generated.h"

UCLASS()
class NEXUS_API ANexusPlayerCharacter : public ANexusCharacterBase
{
	GENERATED_BODY()
public:
	virtual void ApplyTeamVisuals() const override;
	virtual void OnRep_TeamID() override;
};
