
#include "SParticleSystemOptimizationPage.h"
#include "AssetRegistryModule.h"
#include "EngineUtils.h"
#include "Misc/ScopedSlowTask.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "OptimizationAssistantHelpers.h"
#include "OptimizationAssistantGlobalSettings.h"
#include "ParticleSystemOptimizationRules.h"
#include "Particles/ParticleModuleRequired.h"
#include "Particles/ParticleEmitter.h"
#include "Particles/ParticleLODLevel.h"
#include "Particles/Light/ParticleModuleLight.h"
#include "Particles/Spawn/ParticleModuleSpawn.h"
#include "Particles/Spawn/ParticleModuleSpawnPerUnit.h"
#include "Particles/TypeData/ParticleModuleTypeDataRibbon.h"
#include "Game/SilentCheckComponent.h"

SParticleSystemOptimizationPage::SParticleSystemOptimizationPage()
{

}

SParticleSystemOptimizationPage::~SParticleSystemOptimizationPage()
{

}

void SParticleSystemOptimizationPage::Construct(const FArguments& InArgs)
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

	SettingsView->SetObject(GetMutableDefault<UParticleSystemOptimizationRules>());

	ChildSlot
	[
		SettingsView.ToSharedRef()
	];
}

void SParticleSystemOptimizationPage::ProcessOptimizationCheck()
{
	OAHelper::FScopeOutputArchive ScopeOutputArchive(TEXT("ParticleSystemCheckList"));
	RuleSettings = GetMutableDefault<UParticleSystemOptimizationRules>();
	UGlobalCheckSettings* GlobalCheckSettings = GetMutableDefault<UGlobalCheckSettings>();
	
	TArray<UParticleSystem*> ProcessedParticleSystems;
	ProcessedParticleSystems.Add(nullptr); // If UParticleSystem is nullptr, Skip check.

	if (GlobalCheckSettings->OptimizationCheckType == EOptimizationCheckType::OCT_World ||
		GlobalCheckSettings->OptimizationCheckType == EOptimizationCheckType::OCT_WorldDependentAssets)
	{
		const int32 ProgressDenominator = GWorld->GetProgressDenominator();
		FScopedSlowTask SlowTask(ProgressDenominator, FText::FromString(TEXT("Particle System Optimization Check")));
		SlowTask.MakeDialog(true);

		TArray<UParticleSystemComponent*> ProcessedComponents;
		ProcessedComponents.Add(nullptr); // If UParticleSystemComponent is nullptr, Skip check.

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

			TInlineComponentArray<UParticleSystemComponent*> ParticleSystemComponents;
			Actor->GetComponents<UParticleSystemComponent>(ParticleSystemComponents);
			for (UParticleSystemComponent* ParticleComponent : ParticleSystemComponents)
			{
				if (ParticleComponent->IsEditorOnly())
				{
					continue;
				}

				if (ParticleComponent->GetOwner() && ParticleComponent->GetOwner()->IsEditorOnly())
				{
					continue;
				}

				if (ParticleComponent->ComponentHasTag(GlobalCheckSettings->DisableCheckTagName))
				{
					continue;
				}

				if (!ProcessedParticleSystems.Contains(ParticleComponent->Template))
				{
					ProcessedParticleSystems.Add(ParticleComponent->Template);
					ProcessOptimizationCheck(ParticleComponent->Template, *ScopeOutputArchive);
				}

				if (!ProcessedComponents.Contains(ParticleComponent))
				{
					ProcessedComponents.Add(ParticleComponent);
					ProcessOptimizationCheck(ParticleComponent, *ScopeOutputArchive);
				}
			}
		}
	}

	if (GlobalCheckSettings->OptimizationCheckType == EOptimizationCheckType::OCT_AllAssets ||
		GlobalCheckSettings->OptimizationCheckType == EOptimizationCheckType::OCT_WorldDependentAssets)
	{
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		TArray<FAssetData> ParticleSystemList;
		FARFilter Filter;
		Filter.ClassNames.Add(UParticleSystem::StaticClass()->GetFName());
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

		AssetRegistryModule.Get().GetAssets(Filter, ParticleSystemList);

		FScopedSlowTask AssetSlowTask(ParticleSystemList.Num(), FText::FromString(TEXT("Particle System Optimization Check")));
		AssetSlowTask.MakeDialog(true);

		for (int32 Index = 0; Index < ParticleSystemList.Num(); ++Index)
		{
			if (AssetSlowTask.ShouldCancel())
			{
				break;
			}
			AssetSlowTask.EnterProgressFrame(1);

			const FAssetData& AssetData = ParticleSystemList[Index];
			FString Filename = AssetData.ObjectPath.ToString();
			if (Filename.StartsWith("/Game") && !GetMutableDefault<UGlobalCheckSettings>()->IsInNeverCheckDirectory(Filename))
			{
				if (UParticleSystem* ParticleSystem = Cast<UParticleSystem>(AssetData.GetAsset()))
				{
					if (!ProcessedParticleSystems.Contains(ParticleSystem))
					{
						ProcessedParticleSystems.Add(ParticleSystem);
						ProcessOptimizationCheck(ParticleSystem, *ScopeOutputArchive);
					}
				}
			}

			ParticleSystemList.RemoveAtSwap(Index);
			if ((Index % 500) == 0)
			{
				GEngine->TrimMemory();
			}
		}
	}
	GEngine->TrimMemory();
}

void SParticleSystemOptimizationPage::ProcessOptimizationCheck(UParticleSystemComponent* ParticleComponent, FOutputDevice& Ar)
{
	if (RuleSettings->bSkipComponentIfTemplateIsNone && !ParticleComponent->Template)
	{
		return;
	}

	const float PrimitiveSize = ParticleComponent->Bounds.SphereRadius * 2;
	if (PrimitiveSize > RuleSettings->NeverCullParticleSystemSize)
	{
		// 模型较大，不需要设置裁剪距离
		return;
	}

	if (ParticleComponent->bAllowCullDistanceVolume && ParticleComponent->CachedMaxDrawDistance > 0.0f)
	{
		// 受距离裁剪体积控制
		return;
	}

	UGlobalCheckSettings* GlobalCheckSettings = GetMutableDefault<UGlobalCheckSettings>();
	FString ErrorMessage;
	FString ObjectName = ParticleComponent->GetFullName();
	if (ParticleComponent->Template)
	{
		AActor* Actor = ParticleComponent->GetOwner();
		bool bIsReplicated = Actor ? Actor->GetIsReplicated() : false;
		// 网络同步对象，由网络裁剪距离进行裁剪
		if (bIsReplicated)
		{
			if (Actor->NetCullDistanceSquared > (GlobalCheckSettings->MaxNetCullDistanceSquared*GlobalCheckSettings->CullDistanceErrorScale))
			{
				ErrorMessage += FString::Printf(TEXT("设置的网络裁剪距离过大[建议网络裁剪距离小于=%f, 当前裁剪距离=%f].\n"), GlobalCheckSettings->MaxNetCullDistanceSquared, Actor->NetCullDistanceSquared);
				ObjectName = Actor->GetFullName();
			}
		}

		bool bNeverCull = ParticleComponent->bNeverDistanceCull || ParticleComponent->GetLODParentPrimitive();
		if (!bIsReplicated && !bNeverCull)
		{
			bool HasActiveParticlesWithLastLODLevel = true;

			UParticleSystem* ParticleSystem = ParticleComponent->Template;

			if (ParticleSystem->LODDistances.Num() > 0)
			{
				for (auto Emitter : ParticleSystem->Emitters)
				{
					UParticleLODLevel* LODLevel = Emitter->LODLevels[Emitter->LODLevels.Num() - 1];
					if (LODLevel && LODLevel->bEnabled)
					{

						UParticleModuleSpawn* SpawnModule = LODLevel->SpawnModule;
						float MaxSpawnRate = SpawnModule->GetEstimatedSpawnRate();
						int32 MaxBurstCount = SpawnModule->GetMaximumBurstCount();
						for (int32 ModuleIndex = 0; ModuleIndex < LODLevel->Modules.Num(); ModuleIndex++)
						{
							if (UParticleModuleSpawn* SpawnMod = Cast<UParticleModuleSpawn>(LODLevel->Modules[ModuleIndex]))
							{
								MaxSpawnRate += SpawnMod->GetEstimatedSpawnRate();
								MaxBurstCount += SpawnMod->GetMaximumBurstCount();
							} 
							else if (UParticleModuleSpawnPerUnit*ModuleSpawnPerUnit = Cast<UParticleModuleSpawnPerUnit>(LODLevel->Modules[ModuleIndex]))
							{
								MaxSpawnRate += ModuleSpawnPerUnit->GetEstimatedSpawnRate();
								MaxBurstCount += ModuleSpawnPerUnit->GetMaximumBurstCount();
							}
						}
						HasActiveParticlesWithLastLODLevel = (HasActiveParticlesWithLastLODLevel && MaxSpawnRate>0.0f && MaxBurstCount > 0);
					}
				}
			}

			float SilentDistance = 0.0f;
			if (AActor* Owner = ParticleComponent->GetOwner())
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

			FBoxSphereBounds Bounds = ParticleComponent->CalcBounds(ParticleComponent->GetComponentTransform());
			float RecommendDrawDistance = FOptimizationAssistantHelpers::ComputeDrawDistanceFromScreenSize(Bounds.SphereRadius, RuleSettings->ParticleSystemCullScreenSize);
			RecommendDrawDistance *= 2;
			if (SilentDistance > RecommendDrawDistance)
			{
				if (HasActiveParticlesWithLastLODLevel)
				{
					float CachedMaxDrawDistance = FMath::Max(ParticleComponent->CachedMaxDrawDistance, ParticleComponent->LDMaxDrawDistance);
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
				else
				{
					float LastLODDistance = ParticleSystem->LODDistances[ParticleSystem->LODDistances.Num() - 1];
					if (RecommendDrawDistance > 0.0f && LastLODDistance > (RecommendDrawDistance*GlobalCheckSettings->CullDistanceErrorScale))
					{
						ErrorMessage += FString::Printf(TEXT("最后一级LOD距离过大[建议距离=%f,当前距离=%f].\n"), RecommendDrawDistance, LastLODDistance);
					}
				}
			}
		}
	}
	else
	{
		ErrorMessage = FString::Printf(TEXT("无效的粒子系统模板。"));
	}

	if (!ErrorMessage.IsEmpty())
	{
		Ar.Logf(TEXT("%s"), *ObjectName);
		Ar.Logf(TEXT("%s"), *ErrorMessage);
	}
}

void SParticleSystemOptimizationPage::ProcessOptimizationCheck(UParticleSystem* ParticleSystem, FOutputDevice& Ar)
{
	FString ErrorMessage;
	if (ParticleSystem)
	{
		if (ParticleSystem->Emitters.Num() > RuleSettings->MaxEmitterNumber)
		{
			ErrorMessage += FString::Printf(TEXT("粒子系统发射器数超过了限制，最大允许[%d]个发射器, 当前有[%d]个发射器，推荐最多发射器数小于[8]个。\n"), RuleSettings->MaxEmitterNumber, ParticleSystem->Emitters.Num());
		}

		if (ParticleSystem->UpdateTime_FPS > RuleSettings->MaxUpdateTimeFPS)
		{
			ErrorMessage += FString::Printf(TEXT("粒子系统更新频率超过了限制，最大允许[%f]FPS, 当前[%f]FPS, 推荐使用[30]FPS。\n"), RuleSettings->MaxUpdateTimeFPS, ParticleSystem->UpdateTime_FPS);
		}

		for (UParticleEmitter* Emitter : ParticleSystem->Emitters)
		{
			int32 LODLevelIndex = 0;
			for (UParticleLODLevel* ParticleLODLevel : Emitter->LODLevels)
			{
				if (ParticleLODLevel->RequiredModule && ParticleLODLevel->RequiredModule->MaxDrawCount > RuleSettings->MaxParticleCountToDrawForEmitter)
				{
					ErrorMessage += FString::Printf(TEXT("[Emitter=%s,Module=%s,LODLevel=%d]发射的最大粒子数量超过了限制，最大允许粒子数[%d]，当前允许粒子数[MaxDrawCount=%d], 推荐粒子数小于300。\n"), *Emitter->EmitterName.ToString(), *ParticleLODLevel->RequiredModule->GetName(), LODLevelIndex, RuleSettings->MaxParticleCountToDrawForEmitter, ParticleLODLevel->RequiredModule->MaxDrawCount);
				}
				else if (UParticleModuleTypeDataRibbon* ModuleRibbon = Cast<UParticleModuleTypeDataRibbon>(ParticleLODLevel->TypeDataModule))
				{
					if (ModuleRibbon->MaxTrailCount > 500)
					{
						ErrorMessage += FString::Printf(TEXT("[Emitter=%s,Module=%s,LODLevel=%d]发射的最大粒子数量超过了限制，最大允许粒子数[%d]，当前允许粒子数[MaxTrailCount=%d], 推荐粒子数小于300。\n"), *Emitter->EmitterName.ToString(), *ModuleRibbon->GetName(), LODLevelIndex, RuleSettings->MaxParticleCountToDrawForEmitter, ModuleRibbon->MaxTrailCount);
					}

					if (ModuleRibbon->MaxParticleInTrailCount > 500)
					{
						ErrorMessage += FString::Printf(TEXT("[Emitter=%s,Module=%s,LODLevel=%d]发射的最大粒子数量超过了限制，最大允许粒子数[%d]，当前允许粒子数[MaxParticleInTrailCount=%d], 推荐粒子数小于300。\n"), *Emitter->EmitterName.ToString(), *ModuleRibbon->GetName(), LODLevelIndex, RuleSettings->MaxParticleCountToDrawForEmitter, ModuleRibbon->MaxParticleInTrailCount);
					}
				}
				else if (UParticleModuleLight* ParticleModuleLight = Cast<UParticleModuleLight>(ParticleLODLevel))
				{
					if (RuleSettings->bCheckHighQualityLights && ParticleModuleLight->bHighQualityLights)
					{
						ErrorMessage += FString::Printf(TEXT("[Emitter=%s,Module=%s,LODLevel=%d]发射器启用了高质量光照，真的需要吗?\n"), *Emitter->EmitterName.ToString(), *ParticleLODLevel->GetName(), LODLevelIndex);
					}

					if (RuleSettings->bCheckShadowCastingLights && ParticleModuleLight->bShadowCastingLights)
					{
						ErrorMessage += FString::Printf(TEXT("[Emitter=%s,Module=%s,LODLevel=%d]发射器启用了阴影投射，真的需要吗?\n"), *Emitter->EmitterName.ToString(), *ParticleLODLevel->GetName(), LODLevelIndex);
					}
				}
				++LODLevelIndex;
			}
		}
	}
	if (!ErrorMessage.IsEmpty())
	{
		FString ParticleSystemName = ParticleSystem->GetFullName();
		Ar.Logf(TEXT("%s"), *ParticleSystemName);
		Ar.Logf(TEXT("%s"), *ErrorMessage);
	}
}
