
#pragma once
#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "OptimizationAssistantGlobalSettings.h"
#include "StaticMeshOptimizationRules.generated.h"

UCLASS(config = OptimizationAssistant, defaultconfig)
class UStaticMeshOptimizationRules : public UMeshOptimizationRules
{
	GENERATED_BODY()
public:
	UStaticMeshOptimizationRules();
};
