#include "OptimizationAssistantHelpers.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "SceneManagement.h"
#include "PlatformInfo.h"

DEFINE_LOG_CATEGORY(LogOptimizationAssistant);

const PlatformInfo::FPlatformInfo* FOptimizationAssistantHelpers::TargetPlatform = nullptr;
TArray<const PlatformInfo::FPlatformInfo*> FOptimizationAssistantHelpers::AvailablePlatforms;

void FOptimizationAssistantHelpers::GetDependentPackages(const TSet<UPackage*>& RootPackages, TSet<FName>& FoundPackages)
{
	TSet<FName> RootPackageFNames;
	for (const UPackage* RootPackage : RootPackages)
	{
		RootPackageFNames.Add(RootPackage->GetFName());
	}
	GetDependentPackages(RootPackageFNames, FoundPackages);
}

void FOptimizationAssistantHelpers::GetDependentPackages(const TSet<FName>& RootPackages, TSet<FName>& FoundPackages)
{
	TArray<FName> FoundPackagesArray;
	for (const FName& RootPackage : RootPackages)
	{
		FoundPackagesArray.Add(RootPackage);
		FoundPackages.Add(RootPackage);
	}

	// Cache asset registry for later
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry* AssetRegistry = &AssetRegistryModule.Get();

	int FoundPackagesCounter = 0;
	while (FoundPackagesCounter < FoundPackagesArray.Num())
	{
		TArray<FName> PackageDependencies;
		if (AssetRegistry->GetDependencies(FoundPackagesArray[FoundPackagesCounter], PackageDependencies, UE::AssetRegistry::EDependencyCategory::Package) == false)
		{
			UE_LOG(LogOptimizationAssistant, Warning, TEXT("Unable to find package %s in asset registry, cooked asset registry information may be invalid "), *FoundPackagesArray[FoundPackagesCounter].ToString());
		}
		++FoundPackagesCounter;
		for (const FName& OriginalPackageDependency : PackageDependencies)
		{
			// check(PackageDependency.ToString().StartsWith(TEXT("/")));
			FName PackageDependency = OriginalPackageDependency;
			FString PackageDependencyString = PackageDependency.ToString();

			FText OutReason;
			const bool bIncludeReadOnlyRoots = true; // Dependency packages are often script packages (read-only)
			if (!FPackageName::IsValidLongPackageName(PackageDependencyString, bIncludeReadOnlyRoots, &OutReason))
			{
				FString FailMessage = FString::Format(TEXT("Unable to generate long package name for {0}. {1}"), {PackageDependencyString, OutReason.ToString()});
				UE_LOG(LogOptimizationAssistant, Warning, TEXT("%s"), *FailMessage);
				continue;
			}
			else if (FPackageName::IsScriptPackage(PackageDependencyString) || FPackageName::IsMemoryPackage(PackageDependencyString) || FPackageName::IsTempPackage(PackageDependencyString))
			{
				continue;
			}
			else if ()
			{

			}

			if (FoundPackages.Contains(PackageDependency) == false)
			{
				FoundPackages.Add(PackageDependency);
				FoundPackagesArray.Add(PackageDependency);
			}
		}
	}
}

float FOptimizationAssistantHelpers::ComputeDrawDistanceFromScreenSize(const UPrimitiveComponent* PrimitiveComponent, float ScreenSize /*= 0.05f*/, float FOV /*= 90.0f*/)
{
	return ComputeDrawDistanceFromScreenSize(ScreenSize, PrimitiveComponent->Bounds.SphereRadius, FOV);
}

float FOptimizationAssistantHelpers::ComputeDrawDistanceFromScreenSize(float SphereRadius, float ScreenSize /*= 0.05f*/, float FOV /*= 90.0f*/)
{
	static const float FOVRad = FOV * (float)PI / 360.0f;
	static const FMatrix ProjectionMatrix = FPerspectiveMatrix(FOVRad, 1920, 1080, 0.01f);
	return ComputeBoundsDrawDistance(ScreenSize, SphereRadius, ProjectionMatrix);
}

const TArray<const PlatformInfo::FPlatformInfo*>& FOptimizationAssistantHelpers::GetAvailablePlatforms()
{
	if (AvailablePlatforms.Num() == 0)
	{
		// Create and sort a list of vanilla platforms that are game targets (sort by display name)
	// We show all of the platforms regardless of whether we have an SDK installed for them or not
		for (const PlatformInfo::FPlatformInfo& PlatformInfoItem : PlatformInfo::GetPlatformInfoArray())
		{
			if (PlatformInfoItem.IsVanilla() && PlatformInfoItem.PlatformType == EBuildTargetType::Game)
			{
#if !PLATFORM_WINDOWS
				// @todo AllDesktop now only works on Windows (it can compile D3D shaders, and it can remote compile Metal shaders)
				if (PlatformInfoItem.PlatformInfoName != TEXT("AllDesktop"))
#endif
				{
					AvailablePlatforms.Add(&PlatformInfoItem);
				}
			}
		}

		AvailablePlatforms.Sort([](const PlatformInfo::FPlatformInfo& One, const PlatformInfo::FPlatformInfo& Two) -> bool
		{
			return One.DisplayName.CompareTo(Two.DisplayName) < 0;
		});
	}
	return AvailablePlatforms;
}

const PlatformInfo::FPlatformInfo* FOptimizationAssistantHelpers::GetTargetPlatform()
{
	return TargetPlatform;
}

void FOptimizationAssistantHelpers::SetTargetPlatform(const PlatformInfo::FPlatformInfo* InPlatformInfo)
{
	TargetPlatform = InPlatformInfo;
}
