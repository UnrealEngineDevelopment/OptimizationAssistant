// Copyright Epic Games, Inc. All Rights Reserved.

#include "OptimizationAssistantCommands.h"

#define LOCTEXT_NAMESPACE "OptimizationAssistantPlugin"

void FOptimizationAssistantCommands::RegisterCommands()
{
	UI_COMMAND(OpenPluginWindow, "OptimizationAssistant", "Bring up OptimizationAssistant window", EUserInterfaceActionType::Button, FInputGesture());
}

#undef LOCTEXT_NAMESPACE
