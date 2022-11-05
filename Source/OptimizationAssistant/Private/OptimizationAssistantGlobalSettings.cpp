#include "OptimizationAssistantGlobalSettings.h"

UGlobalCheckSettings::UGlobalCheckSettings()
	: Super()
	, DisableCheckTagName(TEXT("TAG_DOAC"))
	, DirectoriesToNeverCheck()
	, MaxNetCullDistanceSquared(15000.f*15000.f)
	, OptimizationFlagsBitmask(OCF_DefaultValue)
	, OptimizationCheckType(EOptimizationCheckType::OCT_None)
	, CullDistanceErrorScale(1.2f)
	, TrianglesErrorScale(1.2f)
{

}

bool UGlobalCheckSettings::IsInNeverCheckDirectory(const FString& InPath)
{
	for (const FDirectoryPath& NeverCheckDirectory : DirectoriesToNeverCheck)
	{
		if (InPath.Contains(NeverCheckDirectory.Path) )
		{
			return true;
		}
	}
	return false;
}

UMeshOptimizationRules::UMeshOptimizationRules()
	: Super()
	, bSkipComponentIfMeshIsNone(true)
	, MinTrianglesNeededForLOD(500)
	, MeshCullScreenSize(0.03f)
	, NeverCullMeshSize(10000.f)
{

}

void UMeshOptimizationRules::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	ValidateSettings();
}

bool UMeshOptimizationRules::ValidateSettings()
{
	FString ErrorMessage;
	return ErrorMessage.IsEmpty();
}