#pragma once

#include "CoreMinimal.h"
#include "UObject/GCObject.h"
#include "Engine/StaticMesh.h"
#include "RecommendMeshSettings.h"

class FEditorStaticMesh : public FGCObject
{
public:
	FEditorStaticMesh();
	~FEditorStaticMesh();
public:
	void Initialize(UStaticMesh* ObjectToEdit);
	void UpdateMeshStats();
	void UpdateLODStats(int32 LODIndex);
	void SetLODCount(int32 LODCount);
	void SetLODScreenSize(float ScreenSize, FName PlatformGroupName, int32 LODIndex = 0);
	void ApplyChanges();
	void ApplyRecommendMeshSettings(URecommendMeshSettings* RecommendMeshSettings, FOutputDevice& Ar);

	UStaticMesh* GetMesh() { return Mesh; };
	TArray<FStaticMaterial> GetMaterials();
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
	/** The currently viewed Static Mesh. */
	UStaticMesh* Mesh;

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
	TArray<FMeshBuildSettings> EditableLODBuildSettings;

	/** Reduction settings to apply when building render data. */
	TArray<FMeshReductionSettings> EditableLODReductionSettings;

	int32 EditableLODCount;

};