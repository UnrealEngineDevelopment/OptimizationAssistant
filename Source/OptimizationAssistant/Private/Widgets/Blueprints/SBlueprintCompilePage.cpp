#include "SBlueprintCompilePage.h"
#include "BlueprintCompileSettings.h"
#include "PropertyEditorModule.h"
#include "Misc/ScopedSlowTask.h"
#include "Modules/ModuleManager.h"
#include "Engine/Blueprint.h"
#include "AssetRegistryModule.h"
#include "Kismet2/CompilerResultsLog.h"
#include "EngineUtils.h"
#include "ISourceControlModule.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Engine/Engine.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "OptimizationAssistantGlobalSettings.h"

DEFINE_LOG_CATEGORY_STATIC(LogCompileAllBlueprints, Log, All);

struct FBlueprintFileInfo
{
	FString  FilePath;
	FDateTime TimestampFileLastModify;

	void Serialize(FArchive& Ar)
	{
		Ar << FilePath;
		Ar << TimestampFileLastModify;
	}

	friend FArchive& operator<<(FArchive& Ar, FBlueprintFileInfo& Ref)
	{
		Ref.Serialize(Ar);
		return Ar;
	}

	bool operator==(const FBlueprintFileInfo& Other) const
	{
		return FilePath == Other.FilePath && TimestampFileLastModify == Other.TimestampFileLastModify;
	}

	bool operator!=(const FBlueprintFileInfo& Other) const
	{
		return FilePath != Other.FilePath || TimestampFileLastModify != Other.TimestampFileLastModify;
	}

	friend uint32 GetTypeHash(const FBlueprintFileInfo& This)
	{
		return HashCombine(GetTypeHash(This.FilePath), GetTypeHash(This.TimestampFileLastModify));
	}
};

class  FBlueprintFileInfoArchive
{
public:
	void Serialize(FArchive& Ar)
	{
		Ar << BlueprintFileInfos;
	}

	friend FArchive& operator<<(FArchive& Ar, FBlueprintFileInfoArchive& Ref)
	{
		Ref.Serialize(Ar);
		return Ar;
	}

	void Empty()
	{
		BlueprintFileInfos.Empty();
	}

	FString GetCompiledRecordPath()
	{
		FString RecordFileDir = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Blueprint"));
		FString RecordFileName = FString::Printf(TEXT("CompiledRecord_%s.bp"), *FEngineVersion::Current().ToString());
		return FPaths::Combine(RecordFileDir, RecordFileName);
	}

	void Save()
	{
		FString FileFullPath = GetCompiledRecordPath();
		TUniquePtr<FArchive> FileWriter(IFileManager::Get().CreateFileWriter(*FileFullPath, FILEWRITE_NoFail));
		*FileWriter << *this;
		FileWriter->Close();
	}

	void Load()
	{
		FString FileFullPath = GetCompiledRecordPath();
		TUniquePtr<FArchive> FileReader(IFileManager::Get().CreateFileReader(*FileFullPath));
		if (FileReader.IsValid())
		{
			*FileReader << *this;
			FileReader->Close();
		}
	}

	void PushCompiledRecordForPackage(const FString& PackageName)
	{
		const FString Extension = FPackageName::GetAssetPackageExtension();
		const FString FilePath = FPackageName::LongPackageNameToFilename(PackageName, Extension);
		const FString FullFilePath = FPaths::ConvertRelativePathToFull(FilePath);
		PushCompiledRecord(FullFilePath);
	}

	void PushCompiledRecord(const FString& InFilePath)
	{
		FBlueprintFileInfo* FoundBPInfo = BlueprintFileInfos.FindByPredicate([&InFilePath](const FBlueprintFileInfo BPInfo)
		{
			return BPInfo.FilePath == InFilePath;
		});

		if (!FoundBPInfo)
		{
			FBlueprintFileInfo FileInfo;
			FileInfo.FilePath = InFilePath;
			FileInfo.TimestampFileLastModify = IFileManager::Get().GetTimeStamp(*InFilePath);
			BlueprintFileInfos.Add(FileInfo);
		}
		else
		{
			FoundBPInfo->TimestampFileLastModify = IFileManager::Get().GetTimeStamp(*InFilePath);
		}
	}

	bool IsNeedCompile(const FString& PackageName)
	{
		const FString Extension = FPackageName::GetAssetPackageExtension();
		const FString FilePath = FPackageName::LongPackageNameToFilename(PackageName, Extension);
		const FString FullFilePath = FPaths::ConvertRelativePathToFull(FilePath);
		FBlueprintFileInfo* FoundBPInfo = BlueprintFileInfos.FindByPredicate([&FullFilePath](const FBlueprintFileInfo BPInfo)
		{
			return BPInfo.FilePath == FullFilePath;
		});

		if (FoundBPInfo)
		{
			if (IFileManager::Get().GetTimeStamp(*FullFilePath) > FoundBPInfo->TimestampFileLastModify)
			{
				return true;
			}
			return false;
		}
		return true;
	}
private:
	TArray<FBlueprintFileInfo> BlueprintFileInfos;
};


SBlueprintCompilePage::SBlueprintCompilePage()
{
	BlueprintBaseClassName = UBlueprint::StaticClass()->GetFName();
	TotalNumFailedLoads = 0;
	TotalNumFatalIssues = 0;
	TotalNumWarnings = 0;
}

SBlueprintCompilePage::~SBlueprintCompilePage()
{

}

void SBlueprintCompilePage::ProcessOptimizationCheck()
{
	BlueprintFileInfoArchive = MakeShared<FBlueprintFileInfoArchive>();
	BlueprintFileInfoArchive->Load();
	RuleSettings = GetMutableDefault<UBlueprintCompileSettings>();
	InitKismetBlueprintCompiler();
	BuildBlueprintAssetList();
	BuildBlueprints();
	LogResults();
	BlueprintFileInfoArchive->Save();
}

void SBlueprintCompilePage::Construct(const FArguments& InArgs)
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

	SettingsView->SetObject(GetMutableDefault<UBlueprintCompileSettings>());

	ChildSlot
	[
		SettingsView.ToSharedRef()
	];
}

void SBlueprintCompilePage::InitKismetBlueprintCompiler()
{
	UE_LOG(LogCompileAllBlueprints, Display, TEXT("Loading Kismit Blueprint Compiler..."));
	//Get Kismet Compiler Setup. Static so that the expensive stuff only happens once per run.
	KismetBlueprintCompilerModule = &FModuleManager::LoadModuleChecked<IKismetCompilerInterface>(TEXT(KISMET_COMPILER_MODULENAME));
	UE_LOG(LogCompileAllBlueprints, Display, TEXT("Finished Loading Kismit Blueprint Compiler..."));
}

void SBlueprintCompilePage::BuildBlueprintAssetList()
{
	UGlobalCheckSettings* GlobalCheckSettings = GetMutableDefault<UGlobalCheckSettings>();

	BlueprintAssetList.Empty();
	if (GlobalCheckSettings->OptimizationCheckType == EOptimizationCheckType::OCT_World)
	{
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);
		for (TObjectIterator<UBlueprint> It; It; ++It)
		{
			UBlueprint* BP = *It;
			if (!BP)
			{
				continue;
			}

			if (UClass* Class = BP->GeneratedClass)
			{
				FAssetData AssetInfo = AssetRegistryModule.Get().GetAssetByObjectPath(*Class->GetPathName());
				BlueprintAssetList.Add(AssetInfo);
			}
		}
	}
	else
	{
		UE_LOG(LogCompileAllBlueprints, Display, TEXT("Loading Asset Registry..."));
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);
		IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

		AssetRegistry.SearchAllAssets(/*bSynchronousSearch =*/true);
		UE_LOG(LogCompileAllBlueprints, Display, TEXT("Finished Loading Asset Registry."));

		UE_LOG(LogCompileAllBlueprints, Display, TEXT("Gathering All Blueprints From Asset Registry..."));
		AssetRegistry.GetAssetsByClass(BlueprintBaseClassName, BlueprintAssetList, true);

		//TArray<FString> PathsToScan;
		//PathsToScan.Add(TEXT("/Game"));
		//AssetRegistry.ScanPathsSynchronous(PathsToScan);
		//TArray<FAssetData> AssetItems;
		//FARFilter Filter;
		//Filter.ClassNames.Add(BlueprintBaseClassName);
		//Filter.PackagePaths.Add(TEXT("/Game"));
		//Filter.bRecursiveClasses = true;
		//Filter.bRecursivePaths = true;
		//AssetRegistry.GetAssets(Filter, AssetItems);
	}
}

void SBlueprintCompilePage::BuildBlueprints()
{
	ErrorsOrWarnings.Empty();
	double LastGCTime = FPlatformTime::Seconds();
	FScopedSlowTask SlowTask(BlueprintAssetList.Num(), FText::FromString(TEXT("Loading and Compiling Blueprints")));
	SlowTask.MakeDialog(true);

	for (int32 Index = BlueprintAssetList.Num() - 1; Index >= 0; --Index)
	{
		if (SlowTask.ShouldCancel())
		{
			break;
		}
		SlowTask.EnterProgressFrame(1);

		const FAssetData& AssetData = BlueprintAssetList[Index];

		if (ShouldBuildAsset(AssetData))
		{
			FString const AssetPath = AssetData.ObjectPath.ToString();
			//UE_LOG(LogCompileAllBlueprints, Display, TEXT("Loading and Compiling: '%s'..."), *AssetPath);

			//Load with LOAD_NoWarn and LOAD_DisableCompileOnLoad as we are covering those explicitly with CompileBlueprint errors.
			UBlueprint* LoadedBlueprint = Cast<UBlueprint>(StaticLoadObject(AssetData.GetClass(), /*Outer =*/nullptr, *AssetPath, nullptr, LOAD_NoWarn | LOAD_DisableCompileOnLoad));
			if (LoadedBlueprint == nullptr)
			{
				++TotalNumFailedLoads;
				UE_LOG(LogCompileAllBlueprints, Error, TEXT("Failed to Load : '%s'."), *AssetPath);
				continue;
			}
			else
			{
				bool bCompileResult = CompileBlueprint(LoadedBlueprint);
				if (bCompileResult && BlueprintFileInfoArchive)
				{
					BlueprintFileInfoArchive->PushCompiledRecordForPackage(AssetData.PackageName.ToString());
				}
			}

			if ((Index % 500) == 0)
			{
				GEngine->TrimMemory();
			}
		}
		BlueprintAssetList.RemoveAtSwap(Index, 1, false);
	}
	GEngine->TrimMemory();
}

bool SBlueprintCompilePage::ShouldBuildAsset(FAssetData const& Asset) const
{
	bool bShouldBuild = true;

	FString Filename = Asset.ObjectPath.ToString();
	if (!Filename.StartsWith("/Game") || GetMutableDefault<UGlobalCheckSettings>()->IsInNeverCheckDirectory(Filename))
	{
		return false;
	}

	if (RuleSettings->IgnoreFolders.Num() > 0)
	{
		for (const FDirectoryPath& IgnoreFolder : RuleSettings->IgnoreFolders)
		{
			if (Asset.ObjectPath.ToString().StartsWith(IgnoreFolder.Path))
			{
				FString const AssetPath = Asset.ObjectPath.ToString();
				UE_LOG(LogCompileAllBlueprints, Verbose, TEXT("Skipping Building %s: As Object is in an Ignored Folder"), *AssetPath);
				bShouldBuild = false;
			}
		}
	}

	if ((RuleSettings->ExcludeAssetTags.Num() > 0) && (CheckHasTagInList(Asset, RuleSettings->ExcludeAssetTags)))
	{
		FString const AssetPath = Asset.ObjectPath.ToString();
		UE_LOG(LogCompileAllBlueprints, Verbose, TEXT("Skipping Building %s: As has an excluded tag"), *AssetPath);
		bShouldBuild = false;
	}

	if ((RuleSettings->RequireAssetTags.Num() > 0) && (!CheckHasTagInList(Asset, RuleSettings->RequireAssetTags)))
	{
		FString const AssetPath = Asset.ObjectPath.ToString();
		UE_LOG(LogCompileAllBlueprints, Verbose, TEXT("Skipping Building %s: As the asset is missing a required tag"), *AssetPath);
		bShouldBuild = false;
	}

	if ((RuleSettings->WhitelistFiles.Num() > 0) && (!CheckInWhitelist(Asset)))
	{
		FString const AssetPath = Asset.ObjectPath.ToString();
		UE_LOG(LogCompileAllBlueprints, Verbose, TEXT("Skipping Building %s: As the asset is not part of the whitelist"), *AssetPath);
		bShouldBuild = false;
	}

	if (RuleSettings->bDirtyOnly)
	{
		const UPackage* AssetPackage = Asset.GetPackage();
		if ((AssetPackage == nullptr) || !AssetPackage->IsDirty())
		{
			FString const AssetPath = Asset.ObjectPath.ToString();
			UE_LOG(LogCompileAllBlueprints, Verbose, TEXT("Skipping Building %s: As Package is not dirty"), *AssetPath);
			bShouldBuild = false;
		}
	}

	if (RuleSettings->IterativeCompiling && BlueprintFileInfoArchive.IsValid())
	{
		bShouldBuild = BlueprintFileInfoArchive->IsNeedCompile(Asset.PackageName.ToString());
	}

	return bShouldBuild;
}

bool SBlueprintCompilePage::CompileBlueprint(UBlueprint* Blueprint)
{
	bool bCompileSucceed = false;
	if (KismetBlueprintCompilerModule && Blueprint)
	{
		//Have to create a new MessageLog for each asset as the warning / error counts are cumulative
		FCompilerResultsLog MessageLog;
		//Need to prevent the Compiler Results Log from automatically outputting results if verbosity is too low
		if (RuleSettings->bResultsOnly)
		{
			MessageLog.bSilentMode = true;
		}
		else
		{
			MessageLog.bAnnotateMentionedNodes = true;
		}
		MessageLog.SetSourcePath(Blueprint->GetPathName());
		MessageLog.BeginEvent(TEXT("Compile"));
		FKismetEditorUtilities::CompileBlueprint(Blueprint, EBlueprintCompileOptions::SkipGarbageCollection, &MessageLog);
		MessageLog.EndEvent();
		bCompileSucceed = (MessageLog.NumErrors == 0);
		if ((MessageLog.NumErrors + MessageLog.NumWarnings) > 0)
		{
			TotalNumFatalIssues += MessageLog.NumErrors;
			TotalNumWarnings += MessageLog.NumWarnings;

			ErrorsOrWarnings.Add(TEXT("\n===============Blueprint With Errors or Warnings:"));
			ErrorsOrWarnings.Add(FString::Printf(TEXT("%s"), *Blueprint->GetPathName()));
			for (TSharedRef<class FTokenizedMessage>& Message : MessageLog.Messages)
			{
				ErrorsOrWarnings.Add(FString::Printf(TEXT("%s"), *Message->ToText().ToString()));
			}
		}

		for (TSharedRef<class FTokenizedMessage>& Message : MessageLog.Messages)
		{
			UE_LOG(LogCompileAllBlueprints, Display, TEXT("%s"), *Message->ToText().ToString());
		}
	}
	return bCompileSucceed;
}

bool SBlueprintCompilePage::CheckHasTagInList(FAssetData const& Asset, const TMap<FString, TArray<FString>>& TagCollectionToCheck) const
{
	bool bContainedTag = false;

	for (const TPair<FString, TArray<FString>>& SingleTagAndValues : TagCollectionToCheck)
	{
		if (Asset.TagsAndValues.Contains(FName(*SingleTagAndValues.Key)))
		{
			const TArray<FString>& TagValuesToCheck = SingleTagAndValues.Value;
			if (TagValuesToCheck.Num() > 0)
			{
				for (const FString& IndividualValueToCheck : TagValuesToCheck)
				{

					if (Asset.TagsAndValues.ContainsKeyValue(FName(*SingleTagAndValues.Key), IndividualValueToCheck))
					{
						bContainedTag = true;
						break;
					}
				}
			}
			//if we don't have any values to check, just return true as the tag was included
			else
			{
				bContainedTag = true;
				break;
			}
		}
	}

	return bContainedTag;
}

bool SBlueprintCompilePage::CheckInWhitelist(FAssetData const& Asset) const
{
	bool bIsInWhitelist = false;

	const FString& AssetFilePath = Asset.ObjectPath.ToString();
	for (const FFilePath& WhiteList : RuleSettings->WhitelistFiles)
	{
		if (AssetFilePath == WhiteList.FilePath)
		{
			bIsInWhitelist = true;
			break;
		}
	}

	return bIsInWhitelist;
}

void SBlueprintCompilePage::LogResults()
{
	//Assets with problems listing
	if ((ErrorsOrWarnings.Num() > 0))
	{
		OAHelper::FScopeOutputArchive ScopeOutputArchive(TEXT("BlueprintCompileIssues"));
		ScopeOutputArchive->Logf(TEXT("Compiling Completed with %d errors and %d warnings and %d blueprints that failed to load.\n"), TotalNumFatalIssues, TotalNumWarnings, TotalNumFailedLoads);
		for (const FString& Asset : ErrorsOrWarnings)
		{
			ScopeOutputArchive->Logf(TEXT("%s"), *Asset);
		}
	}
}
