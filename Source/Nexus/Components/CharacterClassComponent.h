// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CharacterClassComponent.generated.h"
class UCharacterClassInfo;


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class NEXUS_API UCharacterClassComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCharacterClassComponent();

	void ApplyClassFromPlayerState(class ANexusPlayerState* PS);
	void ResetAppliedClass();

	UCharacterClassInfo* GetAppliedClassInfo() const { return AppliedClassInfo; }

protected:
	UPROPERTY()
	TObjectPtr<UCharacterClassInfo> AppliedClassInfo = nullptr;

	UPROPERTY()
	bool bClassApplied = false;
};
