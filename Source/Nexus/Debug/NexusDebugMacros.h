#pragma once

#include "NexusDebugHelper.h"

#define NEXUS_DEBUG(Object, Msg) \
UNexusDebugHelper::DebugMessage(Object, Object, Msg, FColor::Green, 2.0f, true, true)

#define NEXUS_DEBUG_COLOR(Object, Msg, Color) \
UNexusDebugHelper::DebugMessage(Object, Object, Msg, Color, 2.0f, true, true)

#define NEXUS_LOG_ONLY(Object, Msg) \
UNexusDebugHelper::DebugMessage(Object, Object, Msg, FColor::White, 0.0f, false, true)

#define NEXUS_DEBUG_STATE(Object, Label) \
UNexusDebugHelper::DebugMessage(Object, Object, FString::Printf(TEXT("%s"), TEXT(Label)), FColor::Cyan, 2.0f, true, true)