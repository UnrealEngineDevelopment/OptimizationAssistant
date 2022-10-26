#pragma once
#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "OptimizationAssistantGlobalSettings.generated.h"

#define OA_MAX_MESH_LODS 5

enum EOptimizationCheckFlags
{
	OCF_NoFlags = 0x00000000,
	OCF_CullDistance = 0x00000001,
	OCF_NetCullDistance = 0x00000002,
	OCF_MeshLODNum = 0x00000004,
	OCF_MeshLODTriangles = 0x00000008,
	OCF_MeshLODScreenSize = 0x00000010,
	OCF_MeshLODUVChannels = 0x00000020,
	OCF_AnimationFrameRate = 0x00000040,
	OCF_ParticleSystem = 0x00000080,
};


enum class EOptimizationCheckType
{
	OCT_None,
	OCT_World,
	OCT_WorldDependentAssets,
	OCT_AllAssets
};

UCLASS(config = OptimizationAssistant, defaultconfig)
class UGlobalCheckSettings : public UObject
{
	GENERATED_BODY()
public:
	UGlobalCheckSettings();
	UPROPERTY(config, EditAnywhere,meta = (DisplayName = "Directories to never Check", LongPackageName))
	TArray<FDirectoryPath> DirectoriesToNeverCheck;

	/** Square of the max distance from the client's viewpoint that this actor is relevant and will be replicated. */
	UPROPERTY(config, EditAnywhere, Category = Replication)
	float MaxNetCullDistanceSquared;

	EOptimizationCheckType OptimizationCheckType;

	float CullDistanceErrorScale;

	float TrianglesErrorScale;

	bool IsInNeverCheckDirectory(const FString& InPath);
};

USTRUCT()
struct FTriangleLODThresholds
{
	GENERATED_BODY()

	FTriangleLODThresholds():FTriangleLODThresholds(0,0){}

	FTriangleLODThresholds(int32 InTriangles, int32 InLODNum)
		: Triangles(InTriangles)
		, LODNum(InLODNum){}

	UPROPERTY(EditAnywhere, config, meta = (UIMin = "1", UIMax = "50000", ClampMin = "1", ClampMax = "50000"))
	int32 Triangles;

	UPROPERTY(EditAnywhere, config, meta = (UIMin = "1", UIMax = "8", ClampMin = "1", ClampMax = "8"))
	int32 LODNum;
};


UCLASS(config = OptimizationAssistant, defaultconfig)
class UMeshOptimizationRules : public UObject
{
	GENERATED_BODY()
public:
	UMeshOptimizationRules();

	// 如果Mesh组件没有赋予有效的Mesh，则跳过该组件的检查。
	UPROPERTY(EditAnywhere, config)
	bool bSkipComponentIfMeshIsNone;

	UPROPERTY(EditAnywhere, config)
	bool bMeshesDuplicateSections;

	// 模型最多可有的三级面数
	UPROPERTY(EditAnywhere, config, meta = (UIMin = "1", UIMax = "50000", ClampMin = "1", ClampMax = "50000"))
	int32 MaxTriangles;

	// 最多可以使用多少个材质
	UPROPERTY(EditAnywhere, config, meta = (UIMin = "1", UIMax = "16", ClampMin = "1", ClampMax = "16"))
	int32 MaxMaterials;

	// 最多可以使用的UVChannels
	UPROPERTY(EditAnywhere, config, meta = (UIMin = "1", UIMax = "8", ClampMin = "1", ClampMax = "8"))
	int32 MaxUVChannels;

	// 单个LOD最多可使用的材质数
	UPROPERTY(EditAnywhere, config, meta = (UIMin = "1", UIMax = "16", ClampMin = "1", ClampMax = "16"))
	int32 PerLODMaxMaterials;

	// 指定模型小于多少面时不建议为模型创建LOD
	UPROPERTY(EditAnywhere, config, meta = (UIMin = "200", UIMax = "2000", ClampMin = "500", ClampMax = "2000"))
	int32 MinTrianglesNeededForLOD;

	// 当Mesh的屏幕占比到达MeshCullScreenSize的值时，通过ComponentToCamera=MeshComponent_Bounds_SphereRadius / MeshCullScreenSize / Sin(FOV of Camera)
	// 计算出Mesh组件到摄像机的距离，这个距离就是Mesh的标准裁剪距离，如果设置的裁剪距离大于标准裁剪距离会被认为是不合理的。
	UPROPERTY(EditAnywhere, config, meta = (UIMin = "0.005", UIMax = "0.1", ClampMin = "0.005", ClampMax = "0.1"))
	float MeshCullScreenSize;

	// 如果Mesh Size(Component_Bounds_SphereRadius * 2) 大于该值，那么这个Mesh不会受距离裁剪
	UPROPERTY(EditAnywhere, config)
	float NeverCullMeshSize;

	// 配置面数对应的LOD数，即多少面应该至少要有多少级LOD
	UPROPERTY(EditAnywhere, config)
	TArray<FTriangleLODThresholds> TriangleLODThresholds;

	// 配置每级LOD最小Screen Size
	UPROPERTY(EditAnywhere, config, EditFixedSize)
	TArray<float> LODScreenSizeThresholds;

	// 配置每级LOD基于LOD0的面数的百分比
	UPROPERTY(EditAnywhere, config, EditFixedSize)
	TArray<float> BaseLOD0TrianglePercents;

	float GetRecommendLODScreenSize(int32 InLODIndex);

	int32 GetRecommendLODTriangles(int32 InLODIndex, int32 InMaxTriangles);

	float GetRecommendLODTrianglesPercent(int32 InLODIndex);

	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)override;

	virtual bool ValidateSettings();

};
