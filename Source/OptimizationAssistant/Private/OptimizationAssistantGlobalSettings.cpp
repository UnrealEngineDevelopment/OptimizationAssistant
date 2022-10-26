#include "OptimizationAssistantGlobalSettings.h"

UGlobalCheckSettings::UGlobalCheckSettings()
	: Super()
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
	, MaxTriangles(30000)
	, MaxMaterials(16)
	, MaxUVChannels(4)
	, PerLODMaxMaterials(8)
	, MinTrianglesNeededForLOD(500)
	, MeshCullScreenSize(0.03f)
	, NeverCullMeshSize(10000.f)
{
	TriangleLODThresholds.Emplace(5000, 2);
	TriangleLODThresholds.Emplace(10000, 3);
	TriangleLODThresholds.Emplace(20000, 4);
	TriangleLODThresholds.Emplace(30000, 5);

	LODScreenSizeThresholds.Add(1.f);
	LODScreenSizeThresholds.Add(0.3f);
	LODScreenSizeThresholds.Add(0.1f);
	LODScreenSizeThresholds.Add(0.09f);
	LODScreenSizeThresholds.Add(0.07f);

	BaseLOD0TrianglePercents.Add(1.f);
	BaseLOD0TrianglePercents.Add(0.5f);
	BaseLOD0TrianglePercents.Add(0.25f);
	BaseLOD0TrianglePercents.Add(0.125f);
	BaseLOD0TrianglePercents.Add(0.0625f);
}

float UMeshOptimizationRules::GetRecommendLODScreenSize(int32 InLODIndex)
{
	if (InLODIndex >= 0 && InLODIndex < OA_MAX_MESH_LODS)
	{
		return LODScreenSizeThresholds[InLODIndex];
	}
	return -1.f;
}

int32 UMeshOptimizationRules::GetRecommendLODTriangles(int32 InLODIndex, int32 InMaxTriangles)
{
	if (InMaxTriangles > MinTrianglesNeededForLOD && InLODIndex >= 0 && InLODIndex < OA_MAX_MESH_LODS)
	{
		return InMaxTriangles * BaseLOD0TrianglePercents[InLODIndex];
	}
	return InMaxTriangles;
}

float UMeshOptimizationRules::GetRecommendLODTrianglesPercent(int32 InLODIndex)
{
	if (InLODIndex >= 0 && InLODIndex < OA_MAX_MESH_LODS)
	{
		return BaseLOD0TrianglePercents[InLODIndex];
	}
	return -1.f;
}

void UMeshOptimizationRules::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	ValidateSettings();
}

bool UMeshOptimizationRules::ValidateSettings()
{
	FString ErrorMessage;
	{
		int32 LastTriangles = -1;
		int32 LastLODNum = -1;
		for (auto TriangleLODThreshold : TriangleLODThresholds)
		{
			if (TriangleLODThreshold.Triangles > LastTriangles && TriangleLODThreshold.LODNum > LastLODNum)
			{
				LastTriangles = TriangleLODThreshold.Triangles;
				LastLODNum = TriangleLODThreshold.LODNum;
			}
			else
			{
				ErrorMessage += TEXT("MeshLODSetting 属性设置错误：\n");
				ErrorMessage += FString::Printf(TEXT("设置新的 Triangles 必须大于或等于[%d],LODNum必须大于[%d]"), static_cast<int32>(LastTriangles*1.5), LastLODNum);
			}
		}
	}
	if (!ErrorMessage.IsEmpty())
	{
		FText DialogTitle = FText::FromString(TEXT("ValidateSettings"));
		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(ErrorMessage), &DialogTitle);
	}
	return ErrorMessage.IsEmpty();
}