
#pragma once
#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "OptimizationAssistantGlobalSettings.h"
#include "SkeletalMeshOptimizationRules.generated.h"

UCLASS(config = OptimizationAssistant, defaultconfig)
class USkeletalMeshOptimizationRules : public UMeshOptimizationRules
{
	GENERATED_BODY()
public:
	USkeletalMeshOptimizationRules();

	UPROPERTY(EditAnywhere, config, Category = Animation, meta = (UIMin = "24", UIMax = "120", ClampMin = "24", ClampMax = "120"))
	int32 AnimMaxFrameRate;
};