#pragma once
#include "CoreMinimal.h"
#include "Templates/SharedPointer.h"
#include "Widgets/SWidget.h"
#include "Widgets/SCompoundWidget.h"
#include "OptimizationAssistantHelpers.h"

struct FPlatformInfoHolder
{
	FPlatformInfoHolder(){}
	FPlatformInfoHolder(const PlatformInfo::FPlatformInfo* InPlatformInfo):AvailablePlatform(InPlatformInfo){}
	const PlatformInfo::FPlatformInfo* AvailablePlatform;
};

class SOptimizationAssistantView : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SOptimizationAssistantView) { }
	//SLATE_EVENT(FOnClicked, OnCloseClicked)
	//SLATE_EVENT(FOnProfileRun, OnDeleteClicked)
	SLATE_END_ARGS()

public:

	/** Default constructor. */
	SOptimizationAssistantView();

	/** Destructor. */
	~SOptimizationAssistantView();

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs The Slate argument list.
	 * @param InModel The data model.
	 */
	void Construct(const FArguments& InArgs);

	FReply HandleCheckForWorld();
	FReply HandleCheckForWorldAllAssets();
	FReply HandleCheckForAllAssets();
	FReply HandleSaveOptimizationRules();

protected:
	/** Platform combo box */
	TSharedRef<SWidget> GenerateTargetPlatformComboItem(const TSharedPtr<FPlatformInfoHolder> InItem);
	void HandleTargetPlatformComboChanged(const TSharedPtr<FPlatformInfoHolder> Item, ESelectInfo::Type SelectInfo);
	FText GetSelectedPlatformComboText() const;
private:
	void ProcessOptimizationCheck();

	TSharedPtr<class SStaticMeshOptimizationPage> StaticMeshOptimizationPage;
	TSharedPtr<class SSkeletalMeshOptimizationPage> SkeletalMeshOptimizationPage;
	TSharedPtr<class SParticleSystemOptimizationPage> ParticleSystemOptimizationPage;
	TSharedPtr<class SBlueprintCompilePage> BlueprintCompilePage;
	
	ECheckBoxState EnableStaticMeshCheck;
	ECheckBoxState EnableSkeletalMeshCheck;
	ECheckBoxState EnableParticleSystemCheck;
	ECheckBoxState EnableBlueprintCompileCheck;

	TArray<TSharedPtr<FPlatformInfoHolder>> AvailablePlatforms;
};