
#include "SSkeletalMeshOptimizationPage.h"
#include "AssetRegistryModule.h"
#include "EngineUtils.h"
#include "PlatformInfo.h"
#include "SkeletalMeshOptimizationRules.h"
#include "Misc/ScopedSlowTask.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "OptimizationAssistantHelpers.h"
#include "OptimizationAssistantGlobalSettings.h"
#include "Game/SilentCheckComponent.h"
#include "Classes/EditorSkeletalMesh.h"

SSkeletalMeshOptimizationPage::SSkeletalMeshOptimizationPage()
{

}

SSkeletalMeshOptimizationPage::~SSkeletalMeshOptimizationPage()
{

}

void SSkeletalMeshOptimizationPage::Construct(const FArguments& InArgs)
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

	SettingsView->SetObject(GetMutableDefault<USkeletalMeshOptimizationRules>());

	ChildSlot
	[
		SettingsView.ToSharedRef()
	];

	EditorSkeletalMesh = MakeShared<FEditorSkeletalMesh>();
}

int32 SSkeletalMeshOptimizationPage::GetMeshMaxTriangles(USkeletalMesh* SkeletalMesh)
{
	int32 MaxTriangles = 0;
	if (FSkeletalMeshRenderData* MeshRenderData = SkeletalMesh->GetResourceForRendering())
	{
		FSkeletalMeshLODRenderData& FirstLODData = MeshRenderData->LODRenderData[0];
		int32 FirstNumSections = FirstLODData.RenderSections.Num();
		for (int32 SectionIndex = 0; SectionIndex < FirstNumSections; SectionIndex++)
		{
			MaxTriangles += FirstLODData.RenderSections[SectionIndex].NumTriangles;
		}
	}
	return MaxTriangles;
}

void SSkeletalMeshOptimizationPage::ProcessOptimizationCheck()
{
	RuleSettings = GetMutableDefault<USkeletalMeshOptimizationRules>();
	UGlobalCheckSettings* GlobalCheckSettings = GetMutableDefault<UGlobalCheckSettings>();

	OAHelper::FScopeOutputArchive AnimationArchive(TEXT("AnimationCheckList"));
	OAHelper::FScopeOutputArchive SkeletalMeshArchive(TEXT("SkeletalMeshCheckList"));
	
	TArray<USkeletalMesh*> ProcessedMeshes;
	TArray<UAnimSequence*> ProcessedAnims;
	ProcessedMeshes.Add(nullptr); // If USkeletalMesh is nullptr,skip check.
	ProcessedAnims.Add(nullptr);  // If UAnimSequence is nullptr,skip check.

	if (GlobalCheckSettings->OptimizationCheckType == EOptimizationCheckType::OCT_World || 
		GlobalCheckSettings->OptimizationCheckType == EOptimizationCheckType::OCT_WorldDependentAssets)
	{
		const int32 ProgressDenominator = GWorld->GetProgressDenominator();
		FScopedSlowTask SlowTask(ProgressDenominator, FText::FromString(TEXT("Skeletal Mesh Optimization Check")));
		SlowTask.MakeDialog(true);

		TArray<USkeletalMeshComponent*> ProcessedComponents;
		ProcessedComponents.Add(nullptr); // If USkeletalMeshComponent is nullptr, Skip check.

		for (FActorIterator ActorIterator(GWorld); ActorIterator; ++ActorIterator)
		{
			if (SlowTask.ShouldCancel())
			{
				break;
			}
			SlowTask.EnterProgressFrame(1.f);
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

			if (GlobalCheckSettings->IsInNeverCheckDirectory(Actor->GetFullName()))
			{
				continue;
			}

			TInlineComponentArray<USkeletalMeshComponent*> SkeletalMeshComponents;
			Actor->GetComponents<USkeletalMeshComponent>(SkeletalMeshComponents);
			for (USkeletalMeshComponent* MeshComponent : SkeletalMeshComponents)
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

				if (!ProcessedMeshes.Contains(MeshComponent->SkeletalMesh))
				{
					if (GlobalCheckSettings->IsInNeverCheckDirectory(MeshComponent->SkeletalMesh->GetFullName()))
					{
						continue;
					}
					ProcessedMeshes.Add(MeshComponent->SkeletalMesh);
					ProcessOptimizationCheck(MeshComponent->SkeletalMesh, *SkeletalMeshArchive);
				}

				if (!ProcessedComponents.Contains(MeshComponent))
				{
					ProcessedComponents.Add(MeshComponent);
					ProcessOptimizationCheck(MeshComponent, *SkeletalMeshArchive);
				}
			}
		}

		for (TObjectIterator<UAnimSequence> It; It; ++It)
		{
			UAnimSequence* Anim = *It;
			if (!ProcessedAnims.Contains(Anim))
			{
				ProcessedAnims.Add(Anim);
				ProcessOptimizationCheck(Anim, *AnimationArchive);
			}
		}
	}

	if (GlobalCheckSettings->OptimizationCheckType == EOptimizationCheckType::OCT_AllAssets ||
		GlobalCheckSettings->OptimizationCheckType == EOptimizationCheckType::OCT_WorldDependentAssets)
	{
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		TArray<FAssetData> SkeletalMeshList;
		FARFilter Filter;
		Filter.ClassNames.Add(USkeletalMesh::StaticClass()->GetFName());
		Filter.ClassNames.Add(UAnimSequence::StaticClass()->GetFName());
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
		AssetRegistryModule.Get().GetAssets(Filter, SkeletalMeshList);

		FScopedSlowTask AssetSlowTask(SkeletalMeshList.Num(), FText::FromString(TEXT("Skeletal Mesh Optimization Check")));
		AssetSlowTask.MakeDialog(true);

		for (int32 Index = SkeletalMeshList.Num() - 1; Index >= 0; --Index)
		{
			if (AssetSlowTask.ShouldCancel())
			{
				break;
			}
			AssetSlowTask.EnterProgressFrame(1);

			const FAssetData& AssetData = SkeletalMeshList[Index];
			FString Filename = AssetData.ObjectPath.ToString();
			if (Filename.StartsWith("/Game") && !GetMutableDefault<UGlobalCheckSettings>()->IsInNeverCheckDirectory(Filename))
			{
				if (USkeletalMesh* Mesh = Cast<USkeletalMesh>(AssetData.GetAsset()))
				{
					if (!ProcessedMeshes.Contains(Mesh))
					{
						ProcessedMeshes.Add(Mesh);
						ProcessOptimizationCheck(Mesh, *SkeletalMeshArchive);
					}
				}
				else if (UAnimSequence* Anim = Cast<UAnimSequence>(AssetData.GetAsset()))
				{
					if (!ProcessedAnims.Contains(Anim))
					{
						ProcessedAnims.Add(Anim);
						ProcessOptimizationCheck(Anim, *AnimationArchive);
					}
				}
			}

			SkeletalMeshList.RemoveAtSwap(Index, 1, false);
			if ((Index % 500) == 0)
			{
				GEngine->TrimMemory();
			}
		}
	}
	DumpSortedMeshTriangles(ProcessedMeshes);
	GEngine->TrimMemory();
}

void SSkeletalMeshOptimizationPage::ProcessOptimizationCheck(USkeletalMeshComponent* MeshComponent, FOutputDevice& Ar)
{
	if (RuleSettings->bSkipComponentIfMeshIsNone && !MeshComponent->SkeletalMesh)
	{
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

void SSkeletalMeshOptimizationPage::ProcessOptimizationCheck(USkeletalMesh* SkeletalMesh, FOutputDevice& Ar)
{
	if (SkeletalMesh && RuleSettings)
	{
		EditorSkeletalMesh->Initialize(SkeletalMesh);
		FString MeshName = SkeletalMesh->GetFullName();
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
			//Ar.Logf(TEXT(" ========================== ApplyRecommendMeshSettings =============== "));
			//EditorSkeletalMesh->ApplyRecommendMeshSettings(RuleSettings,Ar);
			Ar.Logf(TEXT("%s"), *MeshName);
			Ar.Logf(TEXT("%s"), *ErrorMessage);
		}
	}
}

void SSkeletalMeshOptimizationPage::ProcessOptimizationCheck(UAnimSequence* AnimSequence, FOutputDevice& Ar)
{
	FString AnimSequenceName = AnimSequence->GetFullName();
	FString ErrorMessage;

	if (AnimSequence->ImportResampleFramerate > RuleSettings->AnimMaxFrameRate)
	{
		ErrorMessage += FString::Printf(TEXT("动画更新频率超过了最大限制，允许最大更新频率为[%d]FPS, 当前更新频率为[%d]FPS。\n"), RuleSettings->AnimMaxFrameRate, AnimSequence->ImportResampleFramerate);
	}

	if (!ErrorMessage.IsEmpty())
	{
		Ar.Logf(TEXT("%s"), *AnimSequenceName);
		Ar.Logf(TEXT("%s"), *ErrorMessage);
	}
}

void SSkeletalMeshOptimizationPage::CheckCullDistance(USkeletalMeshComponent * MeshComponent, FString & ErrorMessage)
{
	UGlobalCheckSettings* GlobalCheckSettings = GetMutableDefault<UGlobalCheckSettings>();
	if (!GlobalCheckSettings->HasAnyFlags(EOptimizationCheckFlags::OCF_CullDistance)) return;

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

void SSkeletalMeshOptimizationPage::CheckNetCullDistance(USkeletalMeshComponent * MeshComponent, FString & ErrorMessage)
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

void SSkeletalMeshOptimizationPage::CheckTrianglesLODNum(FString & ErrorMessage)
{
	UGlobalCheckSettings* GlobalCheckSettings = GetMutableDefault<UGlobalCheckSettings>();
	if (!GlobalCheckSettings->HasAnyFlags(EOptimizationCheckFlags::OCF_TrianglesLODNum)) return;

	int32 MaxTriangles = EditorSkeletalMesh->GetNumTriangles();
	int32 NumLODs = EditorSkeletalMesh->GetNumLODs();
	for (int32 ThresholdIndex = RuleSettings->MaxTrianglesForLODNum.Num() - 1; ThresholdIndex >= 0; --ThresholdIndex)
	{
		FTriangleLODThresholds& TriangleLODThreshold = RuleSettings->MaxTrianglesForLODNum[ThresholdIndex];
		if (MaxTriangles >= TriangleLODThreshold.Triangles)
		{
			int32 RecommendLODNum = FMath::Min(TriangleLODThreshold.LODCount, 3);
			if (NumLODs < RecommendLODNum)
			{
				ErrorMessage += FString::Printf(TEXT("[%d]Triangles至少要有[%d]级LOD,当前有[%d]级.\n"), MaxTriangles, RecommendLODNum, NumLODs);
				break;
			}
		}
	}
}

void SSkeletalMeshOptimizationPage::CheckLODNumLimit(FString & ErrorMessage)
{
	UGlobalCheckSettings* GlobalCheckSettings = GetMutableDefault<UGlobalCheckSettings>();
	if (!GlobalCheckSettings->HasAnyFlags(EOptimizationCheckFlags::OCF_LODNumLimit)) return;
	int32 NumLODs = EditorSkeletalMesh->GetNumLODs();
	if (NumLODs > OA_MAX_MESH_LODS)
	{
		ErrorMessage += FString::Printf(TEXT("LOD数量超过了限制，最多可有[%d]级，当前有[%d]级.\n"), OA_MAX_MESH_LODS, NumLODs);
	}
}

void SSkeletalMeshOptimizationPage::CheckLODTrianglesLimit(FString & ErrorMessage)
{
	UGlobalCheckSettings* GlobalCheckSettings = GetMutableDefault<UGlobalCheckSettings>();
	if (!GlobalCheckSettings->HasAnyFlags(EOptimizationCheckFlags::OCF_LODTrianglesLimit)) return;

	int32 MaxTriangles = EditorSkeletalMesh->GetNumTriangles();
	int32 NumLODs = EditorSkeletalMesh->GetNumLODs();
	for (int32 LODIndex = 0; LODIndex < NumLODs; ++LODIndex)
	{
		int32 LODTriangles = EditorSkeletalMesh->GetNumTriangles(LODIndex);
		int32 RecommendLODTriangles = RuleSettings->GetRecommendLODTriangles(LODIndex, MaxTriangles);
		if (LODTriangles > RecommendLODTriangles * GlobalCheckSettings->TrianglesErrorScale)
		{
			ErrorMessage += FString::Printf(TEXT("第[%d]级LOD的 Triangles 不得大于[%d]，当前为[%d],推荐[Base LOD[0] Percent of Triangles=%f].\n"), LODIndex, RecommendLODTriangles, LODTriangles, RuleSettings->GetRecommendLODTrianglesPercent(LODIndex));
		}
	}
}

void SSkeletalMeshOptimizationPage::CheckLODScreenSizeLimit(FString & ErrorMessage)
{
	UGlobalCheckSettings* GlobalCheckSettings = GetMutableDefault<UGlobalCheckSettings>();
	if (!GlobalCheckSettings->HasAnyFlags(EOptimizationCheckFlags::OCF_LODScreenSizeLimit)) return;

	int32 NumLODs = EditorSkeletalMesh->GetNumLODs();
	for (int32 LODIndex = 0; LODIndex < NumLODs; ++LODIndex)
	{
		float LODScreenSize = EditorSkeletalMesh->GetLODScreenSize(NAME_None, LODIndex);
		const PlatformInfo::FPlatformInfo* TargetPlatform = FOptimizationAssistantHelpers::GetTargetPlatform();
		if (TargetPlatform)
		{
			LODScreenSize = EditorSkeletalMesh->GetLODScreenSize(TargetPlatform->PlatformGroupName, LODIndex);
		}
		float RecommendLODScreenSize = RuleSettings->GetRecommendLODScreenSize(TargetPlatform ? TargetPlatform->PlatformGroupName : NAME_None, LODIndex);
		if (LODScreenSize < RecommendLODScreenSize * 0.8f)// 误差值0.2
		{
			ErrorMessage += FString::Printf(TEXT("第[%d]级LOD的 ScreenSize 不得小于[%f],当前是[%f]\n"), LODIndex, RecommendLODScreenSize, LODScreenSize);
		}
	}
}

void SSkeletalMeshOptimizationPage::CheckLODUVChannelLimit(FString & ErrorMessage)
{
	UGlobalCheckSettings* GlobalCheckSettings = GetMutableDefault<UGlobalCheckSettings>();
	if (!GlobalCheckSettings->HasAnyFlags(EOptimizationCheckFlags::OCF_LODUVChannelLimit)) return;
	int32 NumLODs = EditorSkeletalMesh->GetNumLODs();
	for (int32 LODIndex = 0; LODIndex < NumLODs; ++LODIndex)
	{
		int32 UVChannels = EditorSkeletalMesh->GetNumUVChannels(LODIndex);
		if (UVChannels > RuleSettings->MaxUVChannels)
		{
			ErrorMessage += FString::Printf(TEXT("LOD[%d]使用的UV Channels超过了限制[%d]个，当前为[%d]个\n"), LODIndex, RuleSettings->MaxUVChannels, UVChannels);
		}
	}
}

void SSkeletalMeshOptimizationPage::CheckLODMaterialNumLimit(FString & ErrorMessage)
{
	UGlobalCheckSettings* GlobalCheckSettings = GetMutableDefault<UGlobalCheckSettings>();
	if (!GlobalCheckSettings->HasAnyFlags(EOptimizationCheckFlags::OCF_LODMaterialNumLimit)) return;
	int32 NumLODs = EditorSkeletalMesh->GetNumLODs();
	for (int32 LODIndex = 0; LODIndex < NumLODs; ++LODIndex)
	{
		int32 NumSections = EditorSkeletalMesh->GetNumMaterials(LODIndex);
		if (NumSections > RuleSettings->LODMaxMaterials)
		{
			ErrorMessage += FString::Printf(TEXT("LOD[%d]使用最大的材质数量超过了限制[%d]个，当前有[%d]个\n"), LODIndex, RuleSettings->LODMaxMaterials, NumSections);
		}
	}
}

void SSkeletalMeshOptimizationPage::CheckLODDuplicateMaterials(FString & ErrorMessage)
{
	UGlobalCheckSettings* GlobalCheckSettings = GetMutableDefault<UGlobalCheckSettings>();
	if (!GlobalCheckSettings->HasAnyFlags(EOptimizationCheckFlags::OCF_LODDuplicateMaterials)) return;

	int32 NumLODs = EditorSkeletalMesh->GetNumLODs();
	for (int32 LODIndex = 0; LODIndex < NumLODs; ++LODIndex)
	{
		FSkeletalMeshLODRenderData& LODData = EditorSkeletalMesh->GetMesh()->GetResourceForRendering()->LODRenderData[LODIndex];
		TArray<int32> UsedMaterialIndexs;
		for (const FSkelMeshRenderSection& MaterialSection : LODData.RenderSections)
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

void SSkeletalMeshOptimizationPage::CheckMeshMaterialNumLimit(FString & ErrorMessage)
{
	UGlobalCheckSettings* GlobalCheckSettings = GetMutableDefault<UGlobalCheckSettings>();
	if (!GlobalCheckSettings->HasAnyFlags(EOptimizationCheckFlags::OCF_MeshMaterialNumLimit)) return;

	int32 NonLODMaterials = 0;
	for (const FSkeletalMaterial& SkeletalMaterial : EditorSkeletalMesh->GetMaterials())
	{
		if (!SkeletalMaterial.MaterialSlotName.ToString().Contains(TEXT("LOD"), ESearchCase::CaseSensitive))
		{
			++NonLODMaterials;
		}
	}

	if (NonLODMaterials > RuleSettings->MaxMaterials)
	{
		ErrorMessage += FString::Printf(TEXT("Mesh使用的材质数超过了限制[%d]个, 当前为[%d]个.\n"), RuleSettings->MaxMaterials, NonLODMaterials);
	}
}

void SSkeletalMeshOptimizationPage::DumpSortedMeshTriangles(const TArray<USkeletalMesh*>& Meshes)
{
	UGlobalCheckSettings* GlobalCheckSettings = GetMutableDefault<UGlobalCheckSettings>();
	if (!GlobalCheckSettings->HasAnyFlags(EOptimizationCheckFlags::OCF_SortByTriangles)) return;

	OAHelper::FScopeOutputArchive ScopeOutputArchive(TEXT("SkeletalMeshSortedTriangles"));
	FOutputDevice& Ar = *ScopeOutputArchive;

	TMap<USkeletalMesh*, int32> MeshTrianglesMapping;
	for (USkeletalMesh* Mesh : Meshes)
	{
		if(Mesh)
		{
			int32 MeshTriangles = 0;
			if (FSkeletalMeshRenderData* MeshRenderData = Mesh->GetResourceForRendering())
			{
				FSkeletalMeshLODRenderData& LODRenderData = MeshRenderData->LODRenderData[0];
				int32 NumSections = LODRenderData.RenderSections.Num();
				for (int32 SectionIndex = 0; SectionIndex < NumSections; SectionIndex++)
				{
					MeshTriangles += LODRenderData.RenderSections[SectionIndex].NumTriangles;
				}
			}
			MeshTrianglesMapping.Add(Mesh, MeshTriangles);
		}
	}
	MeshTrianglesMapping.ValueSort([](int32 Left, int32 Right) {return Left > Right; });

	Ar.Logf(TEXT("%140s %10s"), TEXT("Object"), TEXT("Triangles"));

	for (TPair<USkeletalMesh*, int32> MeshTrianglesPair : MeshTrianglesMapping)
	{
		FString MeshName = MeshTrianglesPair.Key->GetFullName();
		Ar.Logf(TEXT("%140s %10d"), *MeshName, MeshTrianglesPair.Value);
	}
}
