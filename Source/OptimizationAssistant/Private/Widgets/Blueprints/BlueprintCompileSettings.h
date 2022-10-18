#pragma once
#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "BlueprintCompileSettings.generated.h"

UCLASS(config = OptimizationAssistant, defaultconfig)
class UBlueprintCompileSettings : public UObject
{
	GENERATED_BODY()
public:
	UBlueprintCompileSettings();

	UPROPERTY(EditAnywhere, config)
	bool bResultsOnly;

	UPROPERTY(EditAnywhere, config)
	bool bSimpleAssetList;

	UPROPERTY(EditAnywhere, config)
	bool bCompileSkeletonOnly;

	UPROPERTY(EditAnywhere, config)
	bool bDirtyOnly;

	UPROPERTY(EditAnywhere, config)
	bool IterativeCompiling;

	UPROPERTY(EditAnywhere, config, meta = (LongPackageName))
	TArray<FDirectoryPath> IgnoreFolders;

	UPROPERTY(config, EditAnywhere,meta = (DisplayName = "WhitelistFiles", RelativeToGameContentDir, LongPackageName))
	TArray<FFilePath> WhitelistFiles;

	TMap<FString, TArray<FString>> RequireAssetTags;

	TMap<FString, TArray<FString>> ExcludeAssetTags;
};