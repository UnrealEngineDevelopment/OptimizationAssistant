#pragma once

#include "CoreMinimal.h"
#include "AssetData.h"
#include "Widgets/SCompoundWidget.h"
#include "Editor/KismetCompiler/Public/KismetCompilerModule.h"
#include "IDetailsView.h"

class SBlueprintCompilePage : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SBlueprintCompilePage) { }
	SLATE_END_ARGS()

public:

	/** Default constructor. */
	SBlueprintCompilePage();

	/** Destructor. */
	~SBlueprintCompilePage();

	void ProcessOptimizationCheck();

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs The Slate argument list.
	 */
	void Construct(const FArguments& InArgs);

	/* Loads the Kismet Compiler Module */
	virtual void InitKismetBlueprintCompiler();

	/* Stores FAssetData for every BlueprintAsset inside of the BlueprintAssetList member variable */
	virtual void BuildBlueprintAssetList();

	/* Loads and builds all blueprints in the BlueprintAssetList member variable */
	virtual void BuildBlueprints();

	/* Determines if we should try and Load / Build the asset based on config variables
	* @Param Asset What asset to check
	* @Return True if we should build the asset
	*/
	virtual bool ShouldBuildAsset(FAssetData const& Asset) const;

	/* Handles attempting to compile an individual blueprint asset.
	* @Param Blueprint Asset that is compiled.
	*/
	virtual bool CompileBlueprint(UBlueprint* Blueprint);

	/* Checks if the passed in asset has any tag inside of the passed in tag collection. 
	* @Param Asset  Asset to check all tags of
	* @Param TagCollectionToCheck Which Tag Collection to check
	* @Return True if it contains a match from the tag collection. False otherwise
	*/
	virtual bool CheckHasTagInList(FAssetData const& Asset, const TMap<FString, TArray<FString>>& TagCollectionToCheck) const;

	/* Checks if the passed in asset is included in the white list.
	* @Param Asset  Asset to check against the whitelist
	* @Return True if the asset is in the whitelist. False otherwise.
	*/
	virtual bool CheckInWhitelist(FAssetData const& Asset) const;

	/* Handles outputting the results to the log */
	virtual void LogResults();
private:
	/** Property viewing widget */
	TSharedPtr<IDetailsView>   SettingsView;

	class UBlueprintCompileSettings* RuleSettings;

	FName BlueprintBaseClassName;
	int TotalNumFailedLoads;
	int TotalNumFatalIssues;
	int TotalNumWarnings;
	TArray<FString> ErrorsOrWarnings;

	IKismetCompilerInterface* KismetBlueprintCompilerModule;
	TArray<FAssetData> BlueprintAssetList;
	TSharedPtr<class FBlueprintFileInfoArchive> BlueprintFileInfoArchive;
};

