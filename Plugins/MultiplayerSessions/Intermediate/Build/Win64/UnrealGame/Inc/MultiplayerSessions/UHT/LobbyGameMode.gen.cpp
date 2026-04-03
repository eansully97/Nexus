// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "LobbyGameMode.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
static_assert(!UE_WITH_CONSTINIT_UOBJECT, "This generated code can only be compiled with !UE_WITH_CONSTINIT_OBJECT");
void EmptyLinkFunctionForGeneratedCodeLobbyGameMode() {}

// ********** Begin Cross Module References ********************************************************
ENGINE_API UClass* Z_Construct_UClass_AGameMode();
MULTIPLAYERSESSIONS_API UClass* Z_Construct_UClass_ALobbyGameMode();
MULTIPLAYERSESSIONS_API UClass* Z_Construct_UClass_ALobbyGameMode_NoRegister();
UPackage* Z_Construct_UPackage__Script_MultiplayerSessions();
// ********** End Cross Module References **********************************************************

// ********** Begin Class ALobbyGameMode ***********************************************************
FClassRegistrationInfo Z_Registration_Info_UClass_ALobbyGameMode;
UClass* ALobbyGameMode::GetPrivateStaticClass()
{
	using TClass = ALobbyGameMode;
	if (!Z_Registration_Info_UClass_ALobbyGameMode.InnerSingleton)
	{
		GetPrivateStaticClassBody(
			TClass::StaticPackage(),
			TEXT("LobbyGameMode"),
			Z_Registration_Info_UClass_ALobbyGameMode.InnerSingleton,
			StaticRegisterNativesALobbyGameMode,
			sizeof(TClass),
			alignof(TClass),
			TClass::StaticClassFlags,
			TClass::StaticClassCastFlags(),
			TClass::StaticConfigName(),
			(UClass::ClassConstructorType)InternalConstructor<TClass>,
			(UClass::ClassVTableHelperCtorCallerType)InternalVTableHelperCtorCaller<TClass>,
			UOBJECT_CPPCLASS_STATICFUNCTIONS_FORCLASS(TClass),
			&TClass::Super::StaticClass,
			&TClass::WithinClass::StaticClass
		);
	}
	return Z_Registration_Info_UClass_ALobbyGameMode.InnerSingleton;
}
UClass* Z_Construct_UClass_ALobbyGameMode_NoRegister()
{
	return ALobbyGameMode::GetPrivateStaticClass();
}
struct Z_Construct_UClass_ALobbyGameMode_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
		{ "HideCategories", "Info Rendering MovementReplication Replication Actor Input Movement Collision Rendering HLOD WorldPartition DataLayers Transformation" },
		{ "IncludePath", "LobbyGameMode.h" },
		{ "ModuleRelativePath", "Public/LobbyGameMode.h" },
		{ "ShowCategories", "Input|MouseInput Input|TouchInput" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_PlayersRequiredToTravel_MetaData[] = {
		{ "Category", "Lobby" },
		{ "ModuleRelativePath", "Public/LobbyGameMode.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_GameplayMapPath_MetaData[] = {
		{ "Category", "Lobby" },
		{ "ModuleRelativePath", "Public/LobbyGameMode.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_bTravelStarted_MetaData[] = {
		{ "Category", "Lobby" },
		{ "ModuleRelativePath", "Public/LobbyGameMode.h" },
	};
#endif // WITH_METADATA

// ********** Begin Class ALobbyGameMode constinit property declarations ***************************
	static const UECodeGen_Private::FIntPropertyParams NewProp_PlayersRequiredToTravel;
	static const UECodeGen_Private::FStrPropertyParams NewProp_GameplayMapPath;
	static void NewProp_bTravelStarted_SetBit(void* Obj);
	static const UECodeGen_Private::FBoolPropertyParams NewProp_bTravelStarted;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
// ********** End Class ALobbyGameMode constinit property declarations *****************************
	static UObject* (*const DependentSingletons[])();
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<ALobbyGameMode>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
}; // struct Z_Construct_UClass_ALobbyGameMode_Statics

// ********** Begin Class ALobbyGameMode Property Definitions **************************************
const UECodeGen_Private::FIntPropertyParams Z_Construct_UClass_ALobbyGameMode_Statics::NewProp_PlayersRequiredToTravel = { "PlayersRequiredToTravel", nullptr, (EPropertyFlags)0x0020080000010015, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(ALobbyGameMode, PlayersRequiredToTravel), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_PlayersRequiredToTravel_MetaData), NewProp_PlayersRequiredToTravel_MetaData) };
const UECodeGen_Private::FStrPropertyParams Z_Construct_UClass_ALobbyGameMode_Statics::NewProp_GameplayMapPath = { "GameplayMapPath", nullptr, (EPropertyFlags)0x0020080000010015, UECodeGen_Private::EPropertyGenFlags::Str, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(ALobbyGameMode, GameplayMapPath), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_GameplayMapPath_MetaData), NewProp_GameplayMapPath_MetaData) };
void Z_Construct_UClass_ALobbyGameMode_Statics::NewProp_bTravelStarted_SetBit(void* Obj)
{
	((ALobbyGameMode*)Obj)->bTravelStarted = 1;
}
const UECodeGen_Private::FBoolPropertyParams Z_Construct_UClass_ALobbyGameMode_Statics::NewProp_bTravelStarted = { "bTravelStarted", nullptr, (EPropertyFlags)0x0020080000000014, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(ALobbyGameMode), &Z_Construct_UClass_ALobbyGameMode_Statics::NewProp_bTravelStarted_SetBit, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_bTravelStarted_MetaData), NewProp_bTravelStarted_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_ALobbyGameMode_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_ALobbyGameMode_Statics::NewProp_PlayersRequiredToTravel,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_ALobbyGameMode_Statics::NewProp_GameplayMapPath,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_ALobbyGameMode_Statics::NewProp_bTravelStarted,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_ALobbyGameMode_Statics::PropPointers) < 2048);
// ********** End Class ALobbyGameMode Property Definitions ****************************************
UObject* (*const Z_Construct_UClass_ALobbyGameMode_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_AGameMode,
	(UObject* (*)())Z_Construct_UPackage__Script_MultiplayerSessions,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_ALobbyGameMode_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_ALobbyGameMode_Statics::ClassParams = {
	&ALobbyGameMode::StaticClass,
	"Game",
	&StaticCppClassTypeInfo,
	DependentSingletons,
	nullptr,
	Z_Construct_UClass_ALobbyGameMode_Statics::PropPointers,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	0,
	UE_ARRAY_COUNT(Z_Construct_UClass_ALobbyGameMode_Statics::PropPointers),
	0,
	0x009002ACu,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_ALobbyGameMode_Statics::Class_MetaDataParams), Z_Construct_UClass_ALobbyGameMode_Statics::Class_MetaDataParams)
};
void ALobbyGameMode::StaticRegisterNativesALobbyGameMode()
{
}
UClass* Z_Construct_UClass_ALobbyGameMode()
{
	if (!Z_Registration_Info_UClass_ALobbyGameMode.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_ALobbyGameMode.OuterSingleton, Z_Construct_UClass_ALobbyGameMode_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_ALobbyGameMode.OuterSingleton;
}
DEFINE_VTABLE_PTR_HELPER_CTOR_NS(, ALobbyGameMode);
ALobbyGameMode::~ALobbyGameMode() {}
// ********** End Class ALobbyGameMode *************************************************************

// ********** Begin Registration *******************************************************************
struct Z_CompiledInDeferFile_FID_UnrealProjects_Nexus_Plugins_MultiplayerSessions_Source_MultiplayerSessions_Source_MultiplayerSessions_Public_LobbyGameMode_h__Script_MultiplayerSessions_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_ALobbyGameMode, ALobbyGameMode::StaticClass, TEXT("ALobbyGameMode"), &Z_Registration_Info_UClass_ALobbyGameMode, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(ALobbyGameMode), 402691102U) },
	};
}; // Z_CompiledInDeferFile_FID_UnrealProjects_Nexus_Plugins_MultiplayerSessions_Source_MultiplayerSessions_Source_MultiplayerSessions_Public_LobbyGameMode_h__Script_MultiplayerSessions_Statics 
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_UnrealProjects_Nexus_Plugins_MultiplayerSessions_Source_MultiplayerSessions_Source_MultiplayerSessions_Public_LobbyGameMode_h__Script_MultiplayerSessions_2420687355{
	TEXT("/Script/MultiplayerSessions"),
	Z_CompiledInDeferFile_FID_UnrealProjects_Nexus_Plugins_MultiplayerSessions_Source_MultiplayerSessions_Source_MultiplayerSessions_Public_LobbyGameMode_h__Script_MultiplayerSessions_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_UnrealProjects_Nexus_Plugins_MultiplayerSessions_Source_MultiplayerSessions_Source_MultiplayerSessions_Public_LobbyGameMode_h__Script_MultiplayerSessions_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0,
};
// ********** End Registration *********************************************************************

PRAGMA_ENABLE_DEPRECATION_WARNINGS
