// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "OptimizationAssistantStyle.h"

class FOptimizationAssistantCommands : public TCommands<FOptimizationAssistantCommands>
{
public:

	FOptimizationAssistantCommands()
		: TCommands<FOptimizationAssistantCommands>(TEXT("OptimizationAssistant"), NSLOCTEXT("Contexts", "OptimizationAssistant", "OptimizationAssistant Plugin"), NAME_None, FOptimizationAssistantStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > OpenPluginWindow;
};