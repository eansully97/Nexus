// Fill out your copyright notice in the Description page of Project Settings.


#include "NexusPlayerCharacter.h"

void ANexusPlayerCharacter::ApplyTeamVisuals() const
{
	Super::ApplyTeamVisuals();
	
	if (GetMesh())
	{
		UMaterialInstanceDynamic* DynamicInstance1 = GetMesh()->CreateDynamicMaterialInstance(0);
		UMaterialInstanceDynamic* DynamicInstance2 = GetMesh()->CreateDynamicMaterialInstance(1);

		switch (TeamID)
		{
		case ENexusTeamID::TeamA:
			DynamicInstance1->SetVectorParameterValue(FName("Paint Tint"), TeamAColor1);
			DynamicInstance2->SetVectorParameterValue(FName("Paint Tint"), TeamAColor2);
			break;
		case ENexusTeamID::TeamB:
			DynamicInstance1->SetVectorParameterValue(FName("Paint Tint"), TeamBColor1);
			DynamicInstance2->SetVectorParameterValue(FName("Paint Tint"), TeamBColor2);
		default: ;
		}
	}
}

void ANexusPlayerCharacter::OnRep_TeamID()
{
	Super::OnRep_TeamID();
	ApplyTeamVisuals();
}
