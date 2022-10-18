#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SStaticMeshOptimizationPage : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SStaticMeshOptimizationPage) { }
	SLATE_END_ARGS()

public:

	/** Default constructor. */
	SStaticMeshOptimizationPage();

	/** Destructor. */
	~SStaticMeshOptimizationPage();

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs The Slate argument list.
	 */
	void Construct(const FArguments& InArgs);

	void ProcessOptimizationCheck();

protected:
	void ProcessOptimizationCheck(UStaticMeshComponent* MeshComponent, FOutputDevice& Ar);
	void ProcessOptimizationCheck(UStaticMesh* StaticMesh, FOutputDevice& Ar);
private:
	/** Property viewing widget */
	TSharedPtr<IDetailsView>   SettingsView;

	class UStaticMeshOptimizationRules* RuleSettings;
};

