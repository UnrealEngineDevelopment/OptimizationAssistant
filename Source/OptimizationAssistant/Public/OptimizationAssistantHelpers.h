#pragma once

#include "CoreMinimal.h"
#include "Misc/OutputDeviceArchiveWrapper.h"

DECLARE_LOG_CATEGORY_EXTERN(LogOptimizationAssistant, Log, All);

class FOptimizationAssistantHelpers
{
public:
	/**
	* GetDependentPackages
	* get package dependencies according to the asset registry
	*
	* @param Packages List of packages to use as the root set for dependency checking
	* @param Found return value, all objects which package is dependent on
	*/
	static void GetDependentPackages(const TSet<UPackage*>& RootPackages, TSet<FName>& FoundPackages);

	/**
	* GetDependentPackages
	* get package dependencies according to the asset registry
	*
	* @param Root set of packages to use when looking for dependencies
	* @param FoundPackages list of packages which were found
	*/
	static void GetDependentPackages(const TSet<FName>& RootPackages, TSet<FName>& FoundPackages);
	
	static float ComputeDrawDistanceFromScreenSize(const UPrimitiveComponent* PrimitiveComponent, float ScreenSize = 0.05f, float FOV = 90.0f);
	static float ComputeDrawDistanceFromScreenSize(float SphereRadius, float ScreenSize = 0.05f, float FOV = 90.0f);

	static const TArray<const PlatformInfo::FPlatformInfo*>& GetAvailablePlatforms();

	static const PlatformInfo::FPlatformInfo* GetTargetPlatform();

	static void SetTargetPlatform(const PlatformInfo::FPlatformInfo* InPlatformInfo);

private:
	static TArray<const PlatformInfo::FPlatformInfo*> AvailablePlatforms;

	static const PlatformInfo::FPlatformInfo* TargetPlatform;
};


namespace OAHelper
{
	struct FScopeOutputArchive
	{
		FScopeOutputArchive(const FString& FileName)
		{
			const FString PathName = *(FPaths::ProfilingDir() + TEXT("OptimizationAssistant/"));
			IFileManager::Get().MakeDirectory(*PathName);
			FString Filename = FString::Printf(TEXT("%s_%s%s"), *FileName, *FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")), TEXT(".txt"));
			FString FileFullPath = FPaths::Combine(PathName, Filename);
#if ALLOW_DEBUG_FILES
			FileAr = IFileManager::Get().CreateDebugFileWriter(*FileFullPath);
#else
			FileAr = IFileManager::Get().CreateFileWriter(*FileFullPath);
#endif
			FileArWrapper = new FOutputDeviceArchiveWrapper(FileAr);
		}

		FORCEINLINE FOutputDevice* operator->() const
		{
			return FileArWrapper;
		}

		FORCEINLINE FOutputDevice& operator*() const
		{
			return *FileArWrapper;
		}

		FORCEINLINE FOutputDevice* Get()
		{
			return FileArWrapper;
		}

		~FScopeOutputArchive()
		{
			// Shutdown and free archive resources
			FileArWrapper->TearDown();
			delete FileArWrapper;
			delete FileAr;
		}
	private:
		FArchive* FileAr;
		FOutputDeviceArchiveWrapper* FileArWrapper;
	};
};
