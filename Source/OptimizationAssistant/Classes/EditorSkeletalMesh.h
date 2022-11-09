#pragma once

#include "CoreMinimal.h"
#include "UObject/GCObject.h"
#include "Engine/SkeletalMesh.h"
#include "RecommendMeshSettings.h"

class FEditorSkeletalMesh : public FGCObject
{
public:
	FEditorSkeletalMesh();
	~FEditorSkeletalMesh();
public:
	void Initialize(USkeletalMesh* ObjectToEdit);
	void UpdateMeshStats();
	void UpdateLODStats(int32 LODIndex);
	void SetLODCount(int32 LODCount);
	void SetLODScreenSize(float ScreenSize, FName PlatformGroupName, int32 LODIndex = 0);
	void ApplyChanges();
	void ApplyRecommendMeshSettings(URecommendMeshSettings* RecommendMeshSettings, FOutputDevice& Ar);

	USkeletalMesh* GetMesh() { return Mesh; };
	TArray<FSkeletalMaterial> GetMaterials();
	int32 GetNumTriangles(int32 LODIndex = 0) const;
	int32 GetNumVertices(int32 LODIndex = 0) const;
	int32 GetNumUVChannels(int32 LODIndex = 0) const;
	int32 GetNumMaterials(int32 LODIndex = INDEX_NONE)const; // 当LODLevel为INDEX_NONE时，返回StaticMesh使用的所有材质
	int32 GetNumLODs()const;
	float GetTrianglesPercent(int32 LODIndex = 0)const;
	float GetLODScreenSize(FName PlatformGroupName, int32 LODIndex = 0)const;
	bool  HasVertexColors()const;
public:
	//~ Begin FGCObject Interface
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	//~ End FGCObject Interface

private:
	int32 InternalGetNumTriangles(int32 LODIndex = 0) const;
	int32 InternalGetNumVertices(int32 LODIndex = 0) const;
	/** Restore the LOD imported data if the LOD is no longer reduced */
	void RestoreNonReducedLOD(int32 LODIndex);
private:
	/** The currently viewed Static Mesh. */
	USkeletalMesh* Mesh;

	/** The number of triangles associated with the static mesh LOD. */
	TArray<int32> NumTriangles;

	/** The number of vertices associated with the static mesh LOD. */
	TArray<int32> NumVertices;

	/** The number of used UV channels. */
	TArray<int32> NumUVChannels;

	TArray<int32> NumMaterials;

	/** The display factors at which LODs swap */
	TArray<FPerPlatformFloat> LODScreenSizes;

	/** Settings applied when building the mesh. */
	TArray<FSkeletalMeshBuildSettings> EditableLODBuildSettings;

	/** Reduction settings to apply when building render data. */
	TArray<FSkeletalMeshOptimizationSettings> EditableLODReductionSettings;

	int32 EditableLODCount;

};