#include "EditorStaticMesh.h"
#include "PlatformInfo.h"
#include "StaticMeshAttributes.h"
#include "StaticMeshOperations.h"

FEditorStaticMesh::FEditorStaticMesh()
 : Mesh(nullptr)
{

}

FEditorStaticMesh::~FEditorStaticMesh()
{

}

void FEditorStaticMesh::Initialize(UStaticMesh* ObjectToEdit)
{
	Mesh = ObjectToEdit;
	UpdateMeshStats();
}

void FEditorStaticMesh::UpdateMeshStats()
{
	if (!Mesh)
	{
		return;
	}
	//Init stat arrays.
	const int32 ArraySize = MAX_STATIC_MESH_LODS;

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

	EditableLODCount = Mesh->GetNumLODs();
	for (int32 LODIndex = 0; LODIndex < EditableLODCount; ++LODIndex)
	{
		UpdateLODStats(LODIndex);
	}
}

void FEditorStaticMesh::UpdateLODStats(int32 LODIndex)
{
	NumTriangles[LODIndex] = 0; //-V781
	NumVertices[LODIndex] = 0; //-V781
	NumUVChannels[LODIndex] = 0; //-V781
	NumMaterials[LODIndex] = 0; //-V781

	if (Mesh->RenderData && Mesh->RenderData->LODResources.IsValidIndex(LODIndex))
	{
		FStaticMeshLODResources& LODModel      = Mesh->RenderData->LODResources[LODIndex];
		NumTriangles[LODIndex]                 = LODModel.GetNumTriangles();
		NumVertices[LODIndex]                  = LODModel.GetNumVertices();
		NumUVChannels[LODIndex]                = LODModel.VertexBuffers.StaticMeshVertexBuffer.GetNumTexCoords();
		NumMaterials[LODIndex]                 = Mesh->GetNumSections(LODIndex);
		FStaticMeshSourceModel& SrcModel       = Mesh->GetSourceModel(LODIndex);
		LODScreenSizes[LODIndex]               = SrcModel.ScreenSize;
		EditableLODBuildSettings[LODIndex]     = SrcModel.BuildSettings;
		EditableLODReductionSettings[LODIndex] = SrcModel.ReductionSettings;
	}
}

void FEditorStaticMesh::SetLODCount(int32 LODCount)
{
	EditableLODCount = LODCount;
}

void FEditorStaticMesh::SetLODScreenSize(float ScreenSize, FName PlatformGroupName, int32 LODIndex /*= 0*/)
{
	if (!Mesh->bAutoComputeLODScreenSize)
	{
		// First propagate any changes from the source models to our local scratch.
		for (int32 i = 0; i < Mesh->GetNumSourceModels(); ++i)
		{
			LODScreenSizes[i] = Mesh->GetSourceModel(i).ScreenSize;
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
		for (int32 i = 1; i < MAX_STATIC_MESH_LODS; ++i)
		{
			float MaxValue = FMath::Max(LODScreenSizes[i - 1].Default - MinimumDifferenceInScreenSize, 0.0f);
			LODScreenSizes[i].Default = FMath::Min(LODScreenSizes[i].Default, MaxValue);
		}
	}
}

void FEditorStaticMesh::ApplyChanges()
{
	FlushRenderingCommands();
	Mesh->Modify();
	Mesh->SetNumSourceModels(EditableLODCount);
	for (int32 LODIndex = 0; LODIndex < EditableLODCount; ++LODIndex)
	{
		FStaticMeshSourceModel& SrcModel = Mesh->GetSourceModel(LODIndex);
		SrcModel.BuildSettings = EditableLODBuildSettings[LODIndex];
		SrcModel.ReductionSettings = EditableLODReductionSettings[LODIndex];

		if (LODIndex == 0)
		{
			SrcModel.ScreenSize.Default = 1.0f;
		}
		else
		{
			SrcModel.ScreenSize = LODScreenSizes[LODIndex];
			FStaticMeshSourceModel& PrevModel = Mesh->GetSourceModel(LODIndex - 1);
			if (SrcModel.ScreenSize.Default >= PrevModel.ScreenSize.Default)
			{
				const float DefaultScreenSizeDifference = 0.01f;
				LODScreenSizes[LODIndex].Default = LODScreenSizes[LODIndex - 1].Default - DefaultScreenSizeDifference;

				// Make sure there are no incorrectly overlapping values
				SrcModel.ScreenSize.Default = 1.0f - 0.01f * LODIndex;
			}
		}
	}
	Mesh->PostEditChange();
}

void FEditorStaticMesh::ApplyRecommendMeshSettings(URecommendMeshSettings* RecommendMeshSettings, FOutputDevice& Ar)
{
	bool bIsApplyNeeded = false;
	int32 TempMaxNumTriangles = GetNumTriangles();
	// Check MaxTriangles and fixed.
	if (GetNumTriangles() > RecommendMeshSettings->MaxTriangles)
	{
		float TrianglesPercent = RecommendMeshSettings->MaxTriangles / GetNumTriangles();
		EditableLODReductionSettings[0].PercentTriangles = TrianglesPercent;
		TempMaxNumTriangles = RecommendMeshSettings->MaxTriangles;
		bIsApplyNeeded = true;
	}

	// Check LOD Count and fixed.
	int32 TempLODCount = EditableLODCount;
	for (int32 Index = RecommendMeshSettings->MaxTrianglesForLODNum.Num()-1; Index >= 0; --Index)
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
		float RecommendPercentTriangles = RecommendMeshSettings->GetRecommendLODTrianglesPercent(LODIndex, EditableLODReductionSettings[0].PercentTriangles);
		if (EditableLODReductionSettings[LODIndex].PercentTriangles > RecommendPercentTriangles)
		{
			EditableLODReductionSettings[LODIndex].PercentTriangles = RecommendPercentTriangles;
			bIsApplyNeeded = true;
		}
	}

	// Check LOD ScreenSize and fixed.
	for (LODIndex = 0; LODIndex < GetNumLODs(); ++LODIndex)
	{
		float RecommendScreenSize = RecommendMeshSettings->GetRecommendLODScreenSize(NAME_None,LODIndex);
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

TArray<FStaticMaterial> FEditorStaticMesh::GetMaterials()
{
	return Mesh->StaticMaterials;
}

int32 FEditorStaticMesh::GetNumTriangles(int32 LODIndex /*= 0*/) const
{
	return NumTriangles.IsValidIndex(LODIndex) ? NumTriangles[LODIndex] : -1;
}

int32 FEditorStaticMesh::GetNumVertices(int32 LODIndex /*= 0*/) const
{
	return NumVertices.IsValidIndex(LODIndex) ? NumVertices[LODIndex] : -1;
}

int32 FEditorStaticMesh::GetNumUVChannels(int32 LODIndex /*= 0*/) const
{
	return NumUVChannels.IsValidIndex(LODIndex) ? NumUVChannels[LODIndex] : -1;
}

int32 FEditorStaticMesh::GetNumMaterials(int32 LODIndex /*= INDEX_NONE*/) const
{
	return NumMaterials.IsValidIndex(LODIndex) ? NumMaterials[LODIndex] : Mesh->StaticMaterials.Num();
}

int32 FEditorStaticMesh::GetNumLODs() const
{
	return Mesh->GetNumSourceModels();
}

float FEditorStaticMesh::GetTrianglesPercent(int32 LODIndex /*= 0*/) const
{
	if (Mesh->IsSourceModelValid(LODIndex))
	{
		FStaticMeshSourceModel& SrcModel = Mesh->GetSourceModel(LODIndex);
		return SrcModel.ReductionSettings.PercentTriangles;
	}
	return 1.0f;
}

float FEditorStaticMesh::GetLODScreenSize(FName PlatformGroupName, int32 LODIndex /*= 0*/) const
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

	if (Mesh->bAutoComputeLODScreenSize)
	{
		ScreenSize = Mesh->RenderData->ScreenSize[LODIndex].Default;
	}
	else if (Mesh->IsSourceModelValid(LODIndex))
	{
		ScreenSize = Mesh->GetSourceModel(LODIndex).ScreenSize.Default;
		const float* PlatformScreenSize = Mesh->GetSourceModel(LODIndex).ScreenSize.PerPlatform.Find(PlatformGroupName);
		if (PlatformScreenSize != nullptr)
		{
			ScreenSize = *PlatformScreenSize;
		}
	}
	return ScreenSize;
}

bool  FEditorStaticMesh::HasVertexColors()const
{
	for (int32 LODIndex = 0; LODIndex < Mesh->GetNumSourceModels(); ++LODIndex)
	{
		const FMeshDescription* MeshDescription = Mesh->GetMeshDescription(LODIndex);
		FStaticMeshConstAttributes Attributes(*MeshDescription);
		TVertexInstanceAttributesConstRef<FVector4> VertexInstanceColors = Attributes.GetVertexInstanceColors();
		if (!VertexInstanceColors.IsValid())
		{
			continue;
		}

		for (const FVertexInstanceID VertexInstanceID : MeshDescription->VertexInstances().GetElementIDs())
		{
			FLinearColor VertexInstanceColor(VertexInstanceColors[VertexInstanceID]);
			if (VertexInstanceColor != FLinearColor::White)
			{
				return true;
			}
		}
	}
	return false;
}

void FEditorStaticMesh::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(Mesh);
}
