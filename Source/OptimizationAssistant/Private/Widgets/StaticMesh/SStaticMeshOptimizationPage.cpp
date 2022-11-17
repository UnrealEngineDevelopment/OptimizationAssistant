
#include "SStaticMeshOptimizationPage.h"
#include "AssetRegistryModule.h"
#include "EngineUtils.h"
#include "PlatformInfo.h"
#include "Misc/ScopedSlowTask.h"
#include "OptimizationAssistantHelpers.h"
#include "StaticMeshOptimizationRules.h"
#include "OptimizationAssistantGlobalSettings.h"
#include "Game/SilentCheckComponent.h"
#include "Classes/EditorStaticMesh.h"

SStaticMeshOptimizationPage::SStaticMeshOptimizationPage()
{

}

SStaticMeshOptimizationPage::~SStaticMeshOptimizationPage()
{

}

void SStaticMeshOptimizationPage::Construct(const FArguments& InArgs)
{
	// initialize settings view
	FDetailsViewArgs DetailsViewArgs;
	{
		DetailsViewArgs.bAllowSearch = true;
		DetailsViewArgs.bHideSelectionTip = true;
		DetailsViewArgs.bLockable = false;
		DetailsViewArgs.bSearchInitialKeyFocus = true;
		DetailsViewArgs.bUpdatesFromSelection = false;
		DetailsViewArgs.bShowOptions = true;
		DetailsViewArgs.bShowModifiedPropertiesOption = false;
		DetailsViewArgs.bAllowMultipleTopLevelObjects = true;
		DetailsViewArgs.bShowActorLabel = false;
		DetailsViewArgs.bCustomNameAreaLocation = true;
		DetailsViewArgs.bCustomFilterAreaLocation = true;
		DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
		DetailsViewArgs.bShowPropertyMatrixButton = false;
	}
	SettingsView = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor").CreateDetailView(DetailsViewArgs);

	SettingsView->SetObject(GetMutableDefault<UStaticMeshOptimizationRules>());

	ChildSlot
	[
		SettingsView.ToSharedRef()
	];

	EditorStaticMesh = MakeShared<FEditorStaticMesh>();
}

void SStaticMeshOptimizationPage::ProcessOptimizationCheck()
{
	RuleSettings = GetMutableDefault<UStaticMeshOptimizationRules>();
	if (!RuleSettings->ValidateSettings())
	{
		return;
	}

	OAHelper::FScopeOutputArchive ScopeOutputArchive(TEXT("StaticMeshCheckList"));
	UGlobalCheckSettings* GlobalCheckSettings = GetMutableDefault<UGlobalCheckSettings>();

	TArray<UStaticMesh*> ProcessedMeshes;
	ProcessedMeshes.Add(nullptr); // If UStaticMesh is nullptr,skip check.

	if (GlobalCheckSettings->OptimizationCheckType == EOptimizationCheckType::OCT_World ||
		GlobalCheckSettings->OptimizationCheckType == EOptimizationCheckType::OCT_WorldDependentAssets)
	{
		const int32 ProgressDenominator = GWorld->GetProgressDenominator();
		FScopedSlowTask SlowTask1(ProgressDenominator, FText::FromString(TEXT("Static Mesh Optimization Check")));
		SlowTask1.MakeDialog(true);

		TArray<UStaticMeshComponent*> ProcessedComponents;
		ProcessedComponents.Add(nullptr); // If UStaticMeshComponent is nullptr,skip check.

		for (FActorIterator ActorIterator(GWorld); ActorIterator; ++ActorIterator)
		{
			if (SlowTask1.ShouldCancel())
			{
				break;
			}
			SlowTask1.EnterProgressFrame(1.f);
			//float ProgressPercent = ActorIterator.GetProgressNumerator() / ProgressDenominator;
			AActor* Actor = *ActorIterator;

			if (Actor->IsEditorOnly())
			{
				continue;
			}

			if (Actor->ActorHasTag(GlobalCheckSettings->DisableCheckTagName))
			{
				continue;
			}

			TInlineComponentArray<UStaticMeshComponent*> StaticMeshComponents;
			Actor->GetComponents<UStaticMeshComponent>(StaticMeshComponents, true);
			for (UStaticMeshComponent* MeshComponent : StaticMeshComponents)
			{
				if (MeshComponent->IsEditorOnly())
				{
					continue;
				}

				if (MeshComponent->GetOwner() && MeshComponent->GetOwner()->IsEditorOnly())
				{
					continue;
				}

				if (MeshComponent->ComponentHasTag(GlobalCheckSettings->DisableCheckTagName))
				{
					continue;
				}

				if (!ProcessedMeshes.Contains(MeshComponent->GetStaticMesh()))
				{
					ProcessedMeshes.Add(MeshComponent->GetStaticMesh());
					ProcessOptimizationCheck(MeshComponent->GetStaticMesh(), *ScopeOutputArchive);
				}

				if (!ProcessedComponents.Contains(MeshComponent))
				{
					ProcessedComponents.Add(MeshComponent);
					ProcessOptimizationCheck(MeshComponent, *ScopeOutputArchive);
				}
			}
		}
	}

	if (GlobalCheckSettings->OptimizationCheckType == EOptimizationCheckType::OCT_AllAssets ||
	    GlobalCheckSettings->OptimizationCheckType == EOptimizationCheckType::OCT_WorldDependentAssets)
	{
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		TArray<FAssetData> StaticMeshAssetList;
		FARFilter Filter;
		Filter.ClassNames.Add(UStaticMesh::StaticClass()->GetFName());
		//removed path as a filter as it causes two large lists to be sorted.  Filtering on "game" directory on iteration
		//Filter.PackagePaths.Add("/Game");
		Filter.bRecursiveClasses = true;
		Filter.bRecursivePaths = true;

		if (GlobalCheckSettings->OptimizationCheckType == EOptimizationCheckType::OCT_WorldDependentAssets)
		{
			TSet<FName> DependentPackages;
			TSet<FName> Roots;
			Roots.Add(GWorld->PersistentLevel->GetPackage()->GetFName());
			for (FConstLevelIterator Iterator = GWorld->GetLevelIterator(); Iterator; ++Iterator)
			{
				Roots.Add((*Iterator)->GetPackage()->GetFName());
			}
			FOptimizationAssistantHelpers::GetDependentPackages(Roots, DependentPackages);
			Filter.PackageNames.Append(DependentPackages.Array());
		}

		AssetRegistryModule.Get().GetAssets(Filter, StaticMeshAssetList);

		FScopedSlowTask AssetSlowTask(StaticMeshAssetList.Num(), FText::FromString(TEXT("Static Mesh Optimization Check")));
		AssetSlowTask.MakeDialog(true);

		for (int32 Index = StaticMeshAssetList.Num() - 1; Index >= 0; --Index)
		{
			if (AssetSlowTask.ShouldCancel())
			{
				break;
			}
			AssetSlowTask.EnterProgressFrame(1);
			const FAssetData& AssetData = StaticMeshAssetList[Index];
			FString Filename = AssetData.ObjectPath.ToString();
			if (Filename.StartsWith("/Game") && !GlobalCheckSettings->IsInNeverCheckDirectory(Filename))
			{
				UStaticMesh* StaticMesh = Cast<UStaticMesh>(AssetData.GetAsset());
				if (!ProcessedMeshes.Contains(StaticMesh))
				{
					ProcessedMeshes.Add(StaticMesh);
					ProcessOptimizationCheck(StaticMesh, *ScopeOutputArchive);
				}
			}
			StaticMeshAssetList.RemoveAtSwap(Index, 1, false);
			if ((Index % 500) == 0)
			{
				GEngine->TrimMemory();
			}
		}
	}
	GEngine->TrimMemory();
}

void SStaticMeshOptimizationPage::ProcessOptimizationCheck(UStaticMeshComponent* MeshComponent, FOutputDevice& Ar)
{
	if (RuleSettings->bSkipComponentIfMeshIsNone && !MeshComponent->GetStaticMesh())
	{
		return;
	}

	if (UInstancedStaticMeshComponent* InstancedStaticMeshComponent = Cast<UInstancedStaticMeshComponent>(MeshComponent))
	{
		// UInstancedStaticMeshComponent 不需要检测
		return;
	}

	const float PrimitiveSize = MeshComponent->Bounds.SphereRadius * 2;
	if (PrimitiveSize > RuleSettings->NeverCullMeshSize)
	{
		// 模型较大，不需要设置裁剪距离
		return;
	}

	if (MeshComponent->bAllowCullDistanceVolume && MeshComponent->CachedMaxDrawDistance > 0.0f)
	{
		// 受距离裁剪体积控制
		return;
	}

	if (MeshComponent->bHiddenInGame)
	{
		return;
	}

	FString ErrorMessage;
	CheckCullDistance(MeshComponent, ErrorMessage);
	CheckNetCullDistance(MeshComponent, ErrorMessage);
	if (!ErrorMessage.IsEmpty())
	{
		FString ObjectName = MeshComponent->GetFullName();
		Ar.Logf(TEXT("%s"), *ObjectName);
		Ar.Logf(TEXT("%s"), *ErrorMessage);
	}
}

void SStaticMeshOptimizationPage::ProcessOptimizationCheck(UStaticMesh* StaticMesh, FOutputDevice& Ar)
{
	if (StaticMesh && StaticMesh->RenderData && RuleSettings)
	{
		FString MeshName = StaticMesh->GetFullName();
		bool bAutoGenerated = MeshName.Contains(TEXT("HLOD"), ESearchCase::CaseSensitive) ||
			MeshName.Contains(TEXT("SM_PROXY"), ESearchCase::CaseSensitive) ||
			MeshName.Contains(TEXT("SM_LandscapeStreamingProxy"));

		if (!bAutoGenerated)
		{
			EditorStaticMesh->Initialize(StaticMesh);
			if (EditorStaticMesh->GetNumTriangles() > 500)
			{
				FString ErrorMessage;
				CheckTrianglesLODNum(ErrorMessage);
				CheckLODNumLimit(ErrorMessage);
				CheckLODTrianglesLimit(ErrorMessage);
				CheckLODScreenSizeLimit(ErrorMessage);
				CheckLODUVChannelLimit(ErrorMessage);
				CheckLODMaterialNumLimit(ErrorMessage);
				CheckLODDuplicateMaterials(ErrorMessage);
				CheckMeshMaterialNumLimit(ErrorMessage);
				if (!ErrorMessage.IsEmpty())
				{
					Ar.Logf(TEXT("%s"), *MeshName);
					Ar.Logf(TEXT("%s"), *ErrorMessage);
				}
			}
		}
	}
}

void SStaticMeshOptimizationPage::CheckCullDistance(UStaticMeshComponent * MeshComponent, FString & ErrorMessage)
{
	UGlobalCheckSettings* GlobalCheckSettings = GetMutableDefault<UGlobalCheckSettings>();
	if(!GlobalCheckSettings->HasAnyFlags(EOptimizationCheckFlags::OCF_CullDistance)) return;

	AActor* Actor = MeshComponent->GetOwner();
	bool bIsReplicated = Actor ? Actor->GetIsReplicated() : false;
	bool bNeverCull = MeshComponent->bNeverDistanceCull || MeshComponent->GetLODParentPrimitive();
	if (!bIsReplicated && !bNeverCull)
	{
		float SilentDistance = 0.0f;
		if (AActor* Owner = MeshComponent->GetOwner())
		{
			if (USilentCheckComponent* SilentCheckComponent = Cast<USilentCheckComponent>(Owner->GetComponentByClass(USilentCheckComponent::StaticClass())))
			{
				if (!SilentCheckComponent->bNeverUseSilent)
				{
					SilentDistance = SilentCheckComponent->BreakSilentDistanceSQOverride;
					SilentDistance = FMath::Sqrt(SilentDistance);
				}
			}
		}

		FBoxSphereBounds Bounds = MeshComponent->CalcBounds(MeshComponent->GetComponentTransform());
		float RecommendDrawDistance = FOptimizationAssistantHelpers::ComputeDrawDistanceFromScreenSize(Bounds.SphereRadius, RuleSettings->MeshCullScreenSize);
		float CachedMaxDrawDistance = FMath::Max(MeshComponent->CachedMaxDrawDistance, MeshComponent->LDMaxDrawDistance);

		if (SilentDistance > RecommendDrawDistance)
		{
			if (CachedMaxDrawDistance > 0.0f)
			{
				if (CachedMaxDrawDistance > (RecommendDrawDistance * GlobalCheckSettings->CullDistanceErrorScale))
				{
					ErrorMessage += FString::Printf(TEXT("设置的裁剪距离过大[建议裁剪距离=%f,当前裁剪距离=%f].\n"), RecommendDrawDistance, CachedMaxDrawDistance);
				}
			}
			else if (RecommendDrawDistance > 0.0f && RecommendDrawDistance < 25000.0f)
			{
				// 裁剪距离太近，适当的扩大一点
				RecommendDrawDistance = FMath::Max(RecommendDrawDistance, 1500.0f);
				ErrorMessage += FString::Printf(TEXT("[CachedMaxDrawDistance=%f]未设置有效裁剪距离, 建议裁剪距离=%f.\n"), CachedMaxDrawDistance, RecommendDrawDistance);
			}
		}
	}
}

void SStaticMeshOptimizationPage::CheckNetCullDistance(UStaticMeshComponent * MeshComponent, FString & ErrorMessage)
{
	UGlobalCheckSettings* GlobalCheckSettings = GetMutableDefault<UGlobalCheckSettings>();
	if (!GlobalCheckSettings->HasAnyFlags(EOptimizationCheckFlags::OCF_NetCullDistance)) return;

	AActor* Actor = MeshComponent->GetOwner();
	bool bIsReplicated = Actor ? Actor->GetIsReplicated() : false;
	// 网络同步对象，由网络裁剪距离进行裁剪
	if (bIsReplicated)
	{
		if (Actor->NetCullDistanceSquared > (GlobalCheckSettings->MaxNetCullDistanceSquared))
		{
			float MinNetCullDistanceSquared = 8000.f*8000.f;
			float RecommendNetCullDistanceSquared = FMath::Min(MinNetCullDistanceSquared, GlobalCheckSettings->MaxNetCullDistanceSquared);
			ErrorMessage += FString::Printf(TEXT("设置的网络裁剪距离过大[建议网络裁剪距离小于=%f, 当前裁剪距离=%f].\n"), RecommendNetCullDistanceSquared, Actor->NetCullDistanceSquared);
		}
	}
}

void SStaticMeshOptimizationPage::CheckTrianglesLODNum(FString & ErrorMessage)
{
	UGlobalCheckSettings* GlobalCheckSettings = GetMutableDefault<UGlobalCheckSettings>();
	if (!GlobalCheckSettings->HasAnyFlags(EOptimizationCheckFlags::OCF_TrianglesLODNum)) return;

	int32 NumLODs = EditorStaticMesh->GetNumLODs();
	int32 MaxTriangles = EditorStaticMesh->GetNumTriangles();
	for (int32 ThresholdIndex = RuleSettings->MaxTrianglesForLODNum.Num() - 1; ThresholdIndex >= 0; --ThresholdIndex)
	{
		FTriangleLODThresholds& TriangleLODThreshold = RuleSettings->MaxTrianglesForLODNum[ThresholdIndex];
		if (MaxTriangles >= TriangleLODThreshold.Triangles)
		{
			if (NumLODs < TriangleLODThreshold.LODCount)
			{
				ErrorMessage += FString::Printf(TEXT("[%d]Triangles至少要有[%d]级LOD,当前有[%d]级.\n"), MaxTriangles, TriangleLODThreshold.LODCount, NumLODs);
				break;
			}
		}
	}
}

void SStaticMeshOptimizationPage::CheckLODNumLimit(FString & ErrorMessage)
{
	UGlobalCheckSettings* GlobalCheckSettings = GetMutableDefault<UGlobalCheckSettings>();
	if (!GlobalCheckSettings->HasAnyFlags(EOptimizationCheckFlags::OCF_LODNumLimit)) return;

	int32 NumLODs = EditorStaticMesh->GetNumLODs();
	if (NumLODs > OA_MAX_MESH_LODS)
	{
		ErrorMessage += FString::Printf(TEXT("LOD数量超过了限制，最多可有[%d]级，当前有[%d]级.\n"), OA_MAX_MESH_LODS, NumLODs);
	}
}

void SStaticMeshOptimizationPage::CheckLODTrianglesLimit(FString & ErrorMessage)
{
	UGlobalCheckSettings* GlobalCheckSettings = GetMutableDefault<UGlobalCheckSettings>();
	if (!GlobalCheckSettings->HasAnyFlags(EOptimizationCheckFlags::OCF_LODTrianglesLimit)) return;

	int32 NumLODLevels = EditorStaticMesh->GetNumLODs();
	int32 MaxTriangles = EditorStaticMesh->GetNumTriangles();
	for (int32 LODIndex = 0; LODIndex < NumLODLevels; ++LODIndex)
	{
		int32 LODTriangles = EditorStaticMesh->GetNumTriangles(LODIndex);
		int32 RecommendLODTriangles = RuleSettings->GetRecommendLODTriangles(LODIndex, MaxTriangles);
		if (LODTriangles > RecommendLODTriangles * GlobalCheckSettings->TrianglesErrorScale)
		{
			ErrorMessage += FString::Printf(TEXT("第[%d]级LOD的 Triangles 不得大于[%d]，当前为[%d],推荐基于LOD 0 的Triangles Percent=[%f].\n"), LODIndex, RecommendLODTriangles, LODTriangles, RuleSettings->GetRecommendLODTrianglesPercent(LODIndex));
		}
	}
}

void SStaticMeshOptimizationPage::CheckLODScreenSizeLimit(FString & ErrorMessage)
{
	UGlobalCheckSettings* GlobalCheckSettings = GetMutableDefault<UGlobalCheckSettings>();
	if (!GlobalCheckSettings->HasAnyFlags(EOptimizationCheckFlags::OCF_LODScreenSizeLimit)) return;

	int32 NumLODLevels = EditorStaticMesh->GetNumLODs();
	for (int32 LODIndex = 0; LODIndex < NumLODLevels; ++LODIndex)
	{
		float LODScreenSize = EditorStaticMesh->GetLODScreenSize(NAME_None, LODIndex);
		const PlatformInfo::FPlatformInfo* TargetPlatform = FOptimizationAssistantHelpers::GetTargetPlatform();
		if (TargetPlatform)
		{
			LODScreenSize = EditorStaticMesh->GetLODScreenSize(TargetPlatform->PlatformGroupName, LODIndex);
		}

		float RecommendLODScreenSize = RuleSettings->GetRecommendLODScreenSize(TargetPlatform ? TargetPlatform->PlatformGroupName : NAME_None, LODIndex);
		if (LODScreenSize < (RecommendLODScreenSize - 0.06f))// 误差值0.06
		{
			ErrorMessage += FString::Printf(TEXT("第[%d]级LOD的 ScreenSize 不得小于[%f],当前是[%f]\n"), LODIndex, RecommendLODScreenSize, LODScreenSize);
		}
	}
}

void SStaticMeshOptimizationPage::CheckLODUVChannelLimit(FString & ErrorMessage)
{
	UGlobalCheckSettings* GlobalCheckSettings = GetMutableDefault<UGlobalCheckSettings>();
	if (!GlobalCheckSettings->HasAnyFlags(EOptimizationCheckFlags::OCF_LODUVChannelLimit)) return;

	int32 NumLODLevels = EditorStaticMesh->GetNumLODs();
	for (int32 LODIndex = 0; LODIndex < NumLODLevels; ++LODIndex)
	{
		int32 UVChannels = EditorStaticMesh->GetNumUVChannels(LODIndex);
		if (UVChannels > RuleSettings->MaxUVChannels)
		{
			ErrorMessage += FString::Printf(TEXT("LOD[%d]使用的UV Channels超过了限制[%d]个，当前为[%d]个\n"), LODIndex, RuleSettings->MaxUVChannels, UVChannels);
		}
	}
}

void SStaticMeshOptimizationPage::CheckLODMaterialNumLimit(FString & ErrorMessage)
{
	UGlobalCheckSettings* GlobalCheckSettings = GetMutableDefault<UGlobalCheckSettings>();
	if (!GlobalCheckSettings->HasAnyFlags(EOptimizationCheckFlags::OCF_LODMaterialNumLimit)) return;

	int32 NumLODLevels = EditorStaticMesh->GetNumLODs();
	for (int32 LODIndex = 0; LODIndex < NumLODLevels; ++LODIndex)
	{
		int32 NumMaterials = EditorStaticMesh->GetNumMaterials(LODIndex);
		if (NumMaterials > RuleSettings->LODMaxMaterials)
		{
			ErrorMessage += FString::Printf(TEXT("LOD[%d]使用最大的材质数量超过了限制[%d]个，当前有[%d]个\n"), LODIndex, RuleSettings->LODMaxMaterials, NumMaterials);
		}
	}
}

void SStaticMeshOptimizationPage::CheckLODDuplicateMaterials(FString & ErrorMessage)
{
	UGlobalCheckSettings* GlobalCheckSettings = GetMutableDefault<UGlobalCheckSettings>();
	if (!GlobalCheckSettings->HasAnyFlags(EOptimizationCheckFlags::OCF_LODDuplicateMaterials)) return;

	int32 NumLODLevels = EditorStaticMesh->GetNumLODs();
	for (int32 LODIndex = 0; LODIndex < NumLODLevels; ++LODIndex)
	{
		FStaticMeshLODResources& LODModel = EditorStaticMesh->GetMesh()->RenderData->LODResources[LODIndex];
		TArray<int32> UsedMaterialIndexs;
		for (const FStaticMeshSection& MaterialSection : LODModel.Sections)
		{
			if (UsedMaterialIndexs.Contains(MaterialSection.MaterialIndex))
			{
				ErrorMessage += FString::Printf(TEXT("LOD[%d]使用多个重复的材质，材质索引[%d]\n"), LODIndex, MaterialSection.MaterialIndex);
			}
			else
			{
				UsedMaterialIndexs.Add(MaterialSection.MaterialIndex);
			}
		}
	}
}

void SStaticMeshOptimizationPage::CheckMeshMaterialNumLimit(FString & ErrorMessage)
{
	UGlobalCheckSettings* GlobalCheckSettings = GetMutableDefault<UGlobalCheckSettings>();
	if (!GlobalCheckSettings->HasAnyFlags(EOptimizationCheckFlags::OCF_MeshMaterialNumLimit)) return;

	int32 NonLODMaterials = 0;
	for (const FStaticMaterial&StaticMaterial : EditorStaticMesh->GetMaterials())
	{
		if (!StaticMaterial.MaterialSlotName.ToString().Contains(TEXT("LOD"), ESearchCase::CaseSensitive))
		{
			++NonLODMaterials;
		}
	}

	if (NonLODMaterials > RuleSettings->MaxMaterials)
	{
		ErrorMessage += FString::Printf(TEXT("Mesh使用的材质数超过了限制[%d]个, 当前为[%d]个.\n"), RuleSettings->MaxMaterials, NonLODMaterials);
	}
}
