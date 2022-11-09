#include "RecommendMeshSettings.h"

URecommendMeshSettings::URecommendMeshSettings()
	: MaxTriangles(30000)
	, MaxUVChannels(4)
	, MaxMaterials(16)
	, LODMaxMaterials(8)
	, LODTrianglesPercentDownScale(0.5f)
{
	LODScreenSizes.Add(1.f);
	LODScreenSizes.Add(0.3f);
	LODScreenSizes.Add(0.15f);
	LODScreenSizes.Add(0.1f);
	LODScreenSizes.Add(0.05f);

	MaxTrianglesForLODNum.Emplace(5000, 2);
	MaxTrianglesForLODNum.Emplace(10000, 3);
	MaxTrianglesForLODNum.Emplace(20000, 4);
	MaxTrianglesForLODNum.Emplace(30000, 5);
}

float URecommendMeshSettings::GetRecommendLODScreenSize(FName PlatformGroupName, int32 LODIndex)
{
	const FPerPlatformFloat& LODScreenSize = LODScreenSizes[FMath::Clamp(LODIndex, 0, OA_MAX_MESH_LODS - 1)];
	float ScreenSize = LODScreenSize.Default;
	if (PlatformGroupName != NAME_None)
	{
		const float* PlatformScreenSize = LODScreenSize.PerPlatform.Find(PlatformGroupName);
		if (PlatformScreenSize != nullptr)
		{
			ScreenSize = *PlatformScreenSize;
		}
	}
	return ScreenSize;
}

float URecommendMeshSettings::GetRecommendLODTrianglesPercent(int32 LODIndex, float LOD0TrianglesPercent /*= 1.f*/)
{
	return LODIndex == 0 ? LOD0TrianglesPercent : LOD0TrianglesPercent * (LODTrianglesPercentDownScale / LODIndex);
}

int32 URecommendMeshSettings::GetRecommendLODTriangles(int32 LODIndex, int32 LOD0Triangles, float LOD0TrianglesPercent/* = 1.f*/)
{
	return  LODIndex == 0 ? MaxTriangles : LOD0Triangles * GetRecommendLODTrianglesPercent(LODIndex, LOD0TrianglesPercent);
}

void URecommendMeshSettings::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(FTriangleLODThresholds, Triangles) ||
		PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(FTriangleLODThresholds, LODCount))
	{
		for (int32 i = 1; i < MaxTrianglesForLODNum.Num(); ++i)
		{
			int32 PreTriangles = MaxTrianglesForLODNum[i - 1].Triangles;
			int32 PreLODCount  = MaxTrianglesForLODNum[i - 1].LODCount;
			if (MaxTrianglesForLODNum[i].Triangles < PreTriangles)
			{
				MaxTrianglesForLODNum[i].Triangles = PreTriangles * 1.5f;
			}

			if (MaxTrianglesForLODNum[i].LODCount < PreLODCount && PreLODCount < OA_MAX_MESH_LODS)
			{
				MaxTrianglesForLODNum[i].LODCount = PreLODCount + 1;
			}
		}
	}
}

