#pragma once
#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Classes/RecommendMeshSettings.h"
#include "OptimizationAssistantGlobalSettings.generated.h"

UENUM()
enum EOptimizationCheckFlags
{
	OCF_NoFlags UMETA(Hidden),
	OCF_CullDistance UMETA(DisplayName = "CullDistance"),
	OCF_NetCullDistance UMETA(DisplayName = "NetCullDistance"),
	OCF_TrianglesLODNum UMETA(DisplayName = "TrianglesLODNum"),
	OCF_LODNumLimit UMETA(DisplayName = "LODNumLimit"),
	OCF_LODTrianglesLimit UMETA(DisplayName = "LODTrianglesLimit"),
	OCF_LODScreenSizeLimit UMETA(DisplayName = "LODScreenSizeLimit"),
	OCF_LODMaterialNumLimit UMETA(DisplayName = "LODMaterialNumLimit"),
	OCF_LODUVChannelLimit UMETA(DisplayName = "LODUVChannelLimit"),
	OCF_LODDuplicateMaterials UMETA(DisplayName = "LODDuplicateMaterials"),
	OCF_MeshMaterialNumLimit UMETA(DisplayName = "MeshMaterialNumLimit"),
	OCF_SortByTriangles UMETA(DisplayName = "SortMeshByTriangles"),
	OCF_Max UMETA(Hidden),
};
const int32 OCF_DefaultValue = 0xFFFF;

#define EMUM_TO_FLAG(Enum) (1 << static_cast<uint32>(Enum))

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

	UPROPERTY(VisibleAnywhere)
	FName DisableCheckTagName;
	
	UPROPERTY(config, EditAnywhere,meta = (DisplayName = "Directories to never Check", LongPackageName))
	TArray<FDirectoryPath> DirectoriesToNeverCheck;

	/** Square of the max distance from the client's viewpoint that this actor is relevant and will be replicated. */
	UPROPERTY(config, EditAnywhere, Category = Replication)
	float MaxNetCullDistanceSquared;

	UPROPERTY(config, EditAnywhere, meta = (Bitmask, BitmaskEnum = EOptimizationCheckFlags))
	uint32 OptimizationFlagsBitmask;

	EOptimizationCheckType OptimizationCheckType;

	float CullDistanceErrorScale;

	float TrianglesErrorScale;

	bool IsInNeverCheckDirectory(const FString& InPath);

	bool HasAnyFlags(EOptimizationCheckFlags FlagsToCheck)const
	{
		return (OptimizationFlagsBitmask & EMUM_TO_FLAG(FlagsToCheck)) != 0;
	}
};

UCLASS(config = OptimizationAssistant, defaultconfig)
class UMeshOptimizationRules : public URecommendMeshSettings
{
	GENERATED_BODY()
public:
	UMeshOptimizationRules();

	// 如果Mesh组件没有赋予有效的Mesh，则跳过该组件的检查。
	UPROPERTY(EditAnywhere, config)
	bool bSkipComponentIfMeshIsNone;

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

	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)override;

	virtual bool ValidateSettings();

};
