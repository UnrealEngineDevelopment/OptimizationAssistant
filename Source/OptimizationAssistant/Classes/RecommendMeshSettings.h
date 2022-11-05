#pragma once
#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "RecommendMeshSettings.generated.h"

#define OA_MAX_MESH_LODS 5

USTRUCT()
struct FTriangleLODThresholds
{
	GENERATED_BODY()
	FTriangleLODThresholds() :FTriangleLODThresholds(0, 0) {}
	FTriangleLODThresholds(int32 InTriangles, int32 InLODCount)
		: Triangles(InTriangles)
		, LODCount(InLODCount) {}

	UPROPERTY(EditAnywhere, config, meta = (UIMin = "1", UIMax = "50000", ClampMin = "1", ClampMax = "50000"))
	int32 Triangles;

	UPROPERTY(EditAnywhere, config, meta = (UIMin = "1", UIMax = "8", ClampMin = "1", ClampMax = "8"))
	int32 LODCount;
};

UCLASS(config = OptimizationAssistant, defaultconfig)
class URecommendMeshSettings : public UObject
{
	GENERATED_BODY()
public:
	URecommendMeshSettings();
public:
	// Mesh最多可以有的三角面
	UPROPERTY(EditAnywhere, config, meta = (UIMin = "1", UIMax = "50000", ClampMin = "1", ClampMax = "50000"))
	int32 MaxTriangles;

	// Mesh最多可以使用的UV Channel数
	UPROPERTY(EditAnywhere, config, meta = (UIMin = "1", UIMax = "8", ClampMin = "1", ClampMax = "8"))
	int32 MaxUVChannels;

	// Mesh最多可以使用的材质数
	UPROPERTY(EditAnywhere, config, meta = (UIMin = "1", UIMax = "16", ClampMin = "1", ClampMax = "16"))
	int32 MaxMaterials;

	// 每级LOD最大可使用的材质数
	UPROPERTY(EditAnywhere, config, meta = (UIMin = "1", UIMax = "16", ClampMin = "1", ClampMax = "16", DisplayName = "LOD Max Materials"))
	int32 LODMaxMaterials;

	UPROPERTY(EditAnywhere, config, meta = (UIMin = "0.1", UIMax = "1", ClampMin = "0.1", ClampMax = "1"))
	float LODTrianglesPercentDownScale;

	UPROPERTY(EditAnywhere, config, EditFixedSize)
	TArray<FPerPlatformFloat> LODScreenSizes;

	UPROPERTY(EditAnywhere, config, EditFixedSize)
	TArray<FTriangleLODThresholds> MaxTrianglesForLODNum;

	float GetRecommendLODScreenSize(FName PlatformGroupName, int32 LODIndex);
	float GetRecommendLODTrianglesPercent(int32 LODIndex, float LOD0TrianglesPercent = 1.f);
	int32 GetRecommendLODTriangles(int32 LODIndex, int32 LOD0Triangles, float LOD0TrianglesPercent = 1.f);
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)override;
};