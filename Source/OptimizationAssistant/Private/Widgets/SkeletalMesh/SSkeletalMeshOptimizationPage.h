#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SSkeletalMeshOptimizationPage : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SSkeletalMeshOptimizationPage) { }
	SLATE_END_ARGS()

public:

	/** Default constructor. */
	SSkeletalMeshOptimizationPage();

	/** Destructor. */
	~SSkeletalMeshOptimizationPage();

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs The Slate argument list.
	 */
	void Construct(const FArguments& InArgs);

	void ProcessOptimizationCheck();
protected:
	void ProcessOptimizationCheck(USkeletalMeshComponent* MeshComponent, FOutputDevice& Ar);
	void ProcessOptimizationCheck(USkeletalMesh* SkeletalMesh, FOutputDevice& Ar);
	void ProcessOptimizationCheck(UAnimSequence* AnimSequence, FOutputDevice& Ar);
private:
	/** Property viewing widget */
	TSharedPtr<IDetailsView>   SettingsView;

	class USkeletalMeshOptimizationRules* RuleSettings;
};

