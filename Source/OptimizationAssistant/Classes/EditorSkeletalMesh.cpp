#include "EditorSkeletalMesh.h"
#include "PlatformInfo.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "SkeletalMeshUtilitiesCommon/Public/LODUtilities.h"

FEditorSkeletalMesh::FEditorSkeletalMesh()
	: Mesh(nullptr)
{

}

FEditorSkeletalMesh::~FEditorSkeletalMesh()
{

}

void FEditorSkeletalMesh::Initialize(USkeletalMesh* ObjectToEdit)
{
	Mesh = ObjectToEdit;
	UpdateMeshStats();
}

void FEditorSkeletalMesh::UpdateMeshStats()
{
	if (!Mesh)
	{
		return;
	}
	//Init stat arrays.
	const int32 ArraySize = MAX_MESH_LOD_COUNT;

	NumVertices.Empty(ArraySize);
	NumVertices.AddZeroed(ArraySize);

	NumTriangles.Empty(ArraySize);
	NumTriangles.AddZeroed(ArraySize);

	NumUVChannels.Empty(ArraySize);
	NumUVChannels.AddZeroed(ArraySize);

	NumMaterials.Empty(ArraySize);
	NumMaterials.AddZeroed(ArraySize);

	LODScreenSizes.Empty(ArraySize);
	LODScreenSizes.AddZeroed(ArraySize);

	EditableLODBuildSettings.Empty(ArraySize);
	EditableLODBuildSettings.AddZeroed(ArraySize);

	EditableLODReductionSettings.Empty(ArraySize);
	EditableLODReductionSettings.AddZeroed(ArraySize);

	EditableLODCount = Mesh->GetLODNum();
	for (int32 LODIndex = 0; LODIndex < EditableLODCount; ++LODIndex)
	{
		UpdateLODStats(LODIndex);
	}
}

void FEditorSkeletalMesh::UpdateLODStats(int32 LODIndex)
{
	NumTriangles[LODIndex] = 0; //-V781
	NumVertices[LODIndex] = 0; //-V781
	NumUVChannels[LODIndex] = 0; //-V781
	NumMaterials[LODIndex] = 0; //-V781

	if (Mesh->IsValidLODIndex(LODIndex))
	{
		FSkeletalMeshRenderData* MeshRenderData = Mesh->GetResourceForRendering();
		FSkeletalMeshLODRenderData& LODData     = MeshRenderData->LODRenderData[LODIndex];
		FSkeletalMeshLODInfo* LODInfo           = Mesh->GetLODInfo(LODIndex);
		NumTriangles[LODIndex]                  = InternalGetNumTriangles(LODIndex);
		NumVertices[LODIndex]                   = InternalGetNumVertices(LODIndex);
		NumUVChannels[LODIndex]                 = LODData.GetNumTexCoords();
		NumMaterials[LODIndex]                  = LODData.RenderSections.Num();
		LODScreenSizes[LODIndex]                = LODInfo->ScreenSize;
		EditableLODBuildSettings[LODIndex]      = LODInfo->BuildSettings;
		EditableLODReductionSettings[LODIndex]  = LODInfo->ReductionSettings;
	}
}

void FEditorSkeletalMesh::SetLODCount(int32 LODCount)
{
	EditableLODCount = LODCount;
}

void FEditorSkeletalMesh::SetLODScreenSize(float ScreenSize, FName PlatformGroupName, int32 LODIndex /*= 0*/)
{
	// First propagate any changes from the source models to our local scratch.
	for (int32 i = 0; i < Mesh->GetLODNum(); ++i)
	{
		FSkeletalMeshLODInfo* LODInfo = Mesh->GetLODInfo(LODIndex);
		LODScreenSizes[i] = LODInfo->ScreenSize;
	}

	// Update Display factors for further LODs
	const float MinimumDifferenceInScreenSize = KINDA_SMALL_NUMBER;

	if (PlatformGroupName == NAME_None)
	{
		LODScreenSizes[LODIndex].Default = ScreenSize;
	}
	else
	{
		// Per-platform overrides don't have any restrictions
		float* PlatformScreenSize = LODScreenSizes[LODIndex].PerPlatform.Find(PlatformGroupName);
		if (PlatformScreenSize != nullptr)
		{
			*PlatformScreenSize = ScreenSize;
		}
	}

	// Make sure we aren't trying to overlap or have more than one LOD for a value
	for (int32 i = 1; i < MAX_MESH_LOD_COUNT; ++i)
	{
		float MaxValue = FMath::Max(LODScreenSizes[i - 1].Default - MinimumDifferenceInScreenSize, 0.0f);
		LODScreenSizes[i].Default = FMath::Min(LODScreenSizes[i].Default, MaxValue);
	}
}

void FEditorSkeletalMesh::ApplyChanges()
{
	//Control the scope of the PostEditChange
	FScopedSkeletalMeshPostEditChange ScopedPostEditChange(Mesh);
	// see if there is 
	bool bRegenerateEvenIfImported = false;
	bool bGenerateBaseLOD = false;
	int32 CurrentNumLODs = Mesh->GetLODNum();
	if (CurrentNumLODs == EditableLODCount)
	{
		bool bImportedLODs = false;
		// check if anything is imported and ask if users wants to still regenerate it
		for (int32 LODIndex = 0; LODIndex < EditableLODCount; LODIndex++)
		{
			FSkeletalMeshLODInfo& CurrentLODInfo = *(Mesh->GetLODInfo(LODIndex));
			CurrentLODInfo.ScreenSize = LODScreenSizes[LODIndex];
			CurrentLODInfo.BuildSettings = EditableLODBuildSettings[LODIndex];
			CurrentLODInfo.ReductionSettings = EditableLODReductionSettings[LODIndex];

			bool bIsReductionActive = Mesh->IsReductionActive(LODIndex);

			if (CurrentLODInfo.bHasBeenSimplified == false && bIsReductionActive)
			{
				if (LODIndex > 0)
				{
					bImportedLODs = true;
				}
				else
				{
					bGenerateBaseLOD = true;
				}
			}
			else
			{
				RestoreNonReducedLOD(LODIndex);
			}
		}

		// if LOD is imported, ask users if they want to regenerate or just leave it
		if (bImportedLODs)
		{
			bRegenerateEvenIfImported = true;
		}
	}
	FLODUtilities::RegenerateLOD(Mesh, EditableLODCount, bRegenerateEvenIfImported, bGenerateBaseLOD);
	//PostEditChange will be call when going out of scope

}

void FEditorSkeletalMesh::ApplyRecommendMeshSettings(URecommendMeshSettings* RecommendMeshSettings, FOutputDevice& Ar)
{
	bool bIsApplyNeeded = false;
	int32 TempMaxNumTriangles = GetNumTriangles();
	// Check MaxTriangles and fixed.
	if (GetNumTriangles() > RecommendMeshSettings->MaxTriangles)
	{
		float TrianglesPercent = RecommendMeshSettings->MaxTriangles / GetNumTriangles();
		EditableLODReductionSettings[0].NumOfTrianglesPercentage = TrianglesPercent;
		TempMaxNumTriangles = RecommendMeshSettings->MaxTriangles;
		bIsApplyNeeded = true;
	}

	// Check LOD Count and fixed.
	int32 TempLODCount = EditableLODCount;
	for (int32 Index = RecommendMeshSettings->MaxTrianglesForLODNum.Num() - 1; Index >= 0; --Index)
	{
		if (TempMaxNumTriangles > RecommendMeshSettings->MaxTrianglesForLODNum[Index].Triangles)
		{
			TempLODCount = RecommendMeshSettings->MaxTrianglesForLODNum[Index].LODCount;
			break;
		}
	}
	if (TempLODCount > EditableLODCount)
	{
		EditableLODCount = TempLODCount;
		bIsApplyNeeded = true;
	}

	if (bIsApplyNeeded)
	{
		ApplyChanges();
		UpdateMeshStats();
		bIsApplyNeeded = false;
	}

	FString ErrorMessage;
	int32 LODIndex = 0;
	// Check UVChannels but not fixed, need artists determine.
	for (LODIndex = 0; LODIndex < GetNumLODs(); ++LODIndex)
	{
		if (GetNumUVChannels(LODIndex) > RecommendMeshSettings->MaxUVChannels)
		{
			ErrorMessage += FString::Printf(TEXT("LOD[%d]使用的UV Channels超过了限制[%d]个，当前为[%d]个\n"), LODIndex, RecommendMeshSettings->MaxUVChannels, GetNumUVChannels(LODIndex));
		}
	}

	// Check MaxMaterials but not fixed, need artists determine.
	if (GetNumMaterials() > RecommendMeshSettings->MaxMaterials)
	{
		ErrorMessage += FString::Printf(TEXT("Mesh使用的材质数超过了限制[%d]个, 当前为[%d]个.\n"), RecommendMeshSettings->MaxMaterials, GetNumMaterials());
	}

	// Check LODMaxMaterials but not fixed, need artists determine.
	for (LODIndex = 0; LODIndex < GetNumLODs(); ++LODIndex)
	{
		if (GetNumMaterials(LODIndex) > RecommendMeshSettings->LODMaxMaterials)
		{
			ErrorMessage += FString::Printf(TEXT("LOD[%d]使用最大的材质数量超过了限制[%d]个，当前有[%d]个\n"), LODIndex, RecommendMeshSettings->LODMaxMaterials, GetNumMaterials(LODIndex));
		}
	}

	// Check LOD PercentTriangles and fixed.
	for (LODIndex = 1; LODIndex < GetNumLODs(); ++LODIndex)
	{
		float RecommendPercentTriangles = RecommendMeshSettings->GetRecommendLODTrianglesPercent(LODIndex, EditableLODReductionSettings[0].NumOfTrianglesPercentage);
		if (EditableLODReductionSettings[LODIndex].NumOfTrianglesPercentage > RecommendPercentTriangles)
		{
			EditableLODReductionSettings[LODIndex].NumOfTrianglesPercentage = RecommendPercentTriangles;
			bIsApplyNeeded = true;
		}
	}

	// Check LOD ScreenSize and fixed.
	for (LODIndex = 0; LODIndex < GetNumLODs(); ++LODIndex)
	{
		float RecommendScreenSize = RecommendMeshSettings->GetRecommendLODScreenSize(NAME_None, LODIndex);
		if (GetLODScreenSize(NAME_None, LODIndex) < RecommendScreenSize)
		{
			SetLODScreenSize(RecommendScreenSize, NAME_None, LODIndex);
			bIsApplyNeeded = true;
		}

	}

	if (!ErrorMessage.IsEmpty())
	{
		FString MeshName = Mesh->GetFullName();
		Ar.Logf(TEXT("%s"), *MeshName);
		Ar.Logf(TEXT("%s"), *ErrorMessage);
	}

	if (bIsApplyNeeded)
	{
		ApplyChanges();
		UpdateMeshStats();
		bIsApplyNeeded = false;
	}
}

TArray<FSkeletalMaterial> FEditorSkeletalMesh::GetMaterials()
{
	return Mesh->Materials;
}

int32 FEditorSkeletalMesh::GetNumTriangles(int32 LODIndex /*= 0*/) const
{
	return NumTriangles.IsValidIndex(LODIndex) ? NumTriangles[LODIndex] : -1;
}

int32 FEditorSkeletalMesh::GetNumVertices(int32 LODIndex) const
{
	return NumVertices.IsValidIndex(LODIndex) ? NumVertices[LODIndex] : -1;
}

int32 FEditorSkeletalMesh::GetNumUVChannels(int32 LODIndex) const
{
	return NumUVChannels.IsValidIndex(LODIndex) ? NumUVChannels[LODIndex] : -1;
}

int32 FEditorSkeletalMesh::GetNumMaterials(int32 LODIndex) const
{
	return NumMaterials.IsValidIndex(LODIndex) ? NumMaterials[LODIndex] : Mesh->Materials.Num();
}

int32 FEditorSkeletalMesh::GetNumLODs() const
{
	return Mesh->GetLODNum();
}

float FEditorSkeletalMesh::GetTrianglesPercent(int32 LODIndex /*= 0*/) const
{
	if (Mesh->IsValidLODIndex(LODIndex))
	{
		FSkeletalMeshOptimizationSettings ReductionSettings = Mesh->GetReductionSettings(LODIndex);
		return ReductionSettings.NumOfTrianglesPercentage;
	}
	return 1.0f;
}

float FEditorSkeletalMesh::GetLODScreenSize(FName PlatformGroupName, int32 LODIndex /*= 0*/) const
{
	const FPerPlatformFloat& LODScreenSize = LODScreenSizes[FMath::Clamp(LODIndex, 0, MAX_STATIC_MESH_LODS - 1)];
	float ScreenSize = LODScreenSize.Default;
	if (PlatformGroupName != NAME_None)
	{
		const float* PlatformScreenSize = LODScreenSize.PerPlatform.Find(PlatformGroupName);
		if (PlatformScreenSize != nullptr)
		{
			ScreenSize = *PlatformScreenSize;
		}
	}

	if (Mesh->IsValidLODIndex(LODIndex))
	{
		FSkeletalMeshLODInfo* LODInfo = Mesh->GetLODInfo(LODIndex);
		ScreenSize = LODInfo->ScreenSize.Default;
		const float* PlatformScreenSize = LODInfo->ScreenSize.PerPlatform.Find(PlatformGroupName);
		if (PlatformScreenSize != nullptr)
		{
			ScreenSize = *PlatformScreenSize;
		}
	}
	return ScreenSize;
}

bool FEditorSkeletalMesh::HasVertexColors() const
{
	return Mesh->bHasVertexColors;
}

void FEditorSkeletalMesh::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(Mesh);
}

int32 FEditorSkeletalMesh::InternalGetNumTriangles(int32 LODIndex /*= 0*/) const
{
	int32 MaxTriangles = 0;
	if (FSkeletalMeshRenderData* MeshRenderData = Mesh->GetResourceForRendering())
	{
		FSkeletalMeshLODRenderData& LODRenderData = MeshRenderData->LODRenderData[LODIndex];
		int32 NumSections = LODRenderData.RenderSections.Num();
		for (int32 SectionIndex = 0; SectionIndex < NumSections; SectionIndex++)
		{
			MaxTriangles += LODRenderData.RenderSections[SectionIndex].NumTriangles;
		}
	}
	return MaxTriangles;
}

int32 FEditorSkeletalMesh::InternalGetNumVertices(int32 LODIndex /*= 0*/) const
{
	int32 MaxVertices = 0;
	if (FSkeletalMeshRenderData* MeshRenderData = Mesh->GetResourceForRendering())
	{
		FSkeletalMeshLODRenderData& LODRenderData = MeshRenderData->LODRenderData[LODIndex];
		int32 NumSections = LODRenderData.RenderSections.Num();
		for (int32 SectionIndex = 0; SectionIndex < NumSections; SectionIndex++)
		{
			MaxVertices += LODRenderData.RenderSections[SectionIndex].NumVertices;
		}
	}
	return MaxVertices;
}

void FEditorSkeletalMesh::RestoreNonReducedLOD(int32 LODIndex)
{
	FSkeletalMeshLODInfo* CurrentLODInfo = Mesh->GetLODInfo(LODIndex);
	const bool bIsReductionActive = Mesh->IsReductionActive(LODIndex);
	const bool bIsLODModelbuildDataAvailable = Mesh->IsLODImportedDataBuildAvailable(LODIndex);

	if (CurrentLODInfo->bHasBeenSimplified
		&& !bIsReductionActive
		&& (bIsLODModelbuildDataAvailable
			|| FLODUtilities::RestoreSkeletalMeshLODImportedData(Mesh, LODIndex)))
	{
		CurrentLODInfo->bHasBeenSimplified = false;
	}
}
