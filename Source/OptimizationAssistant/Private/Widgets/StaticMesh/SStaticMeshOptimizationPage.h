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

	void CheckCullDistance(UStaticMeshComponent* MeshComponent, FString& ErrorMessage);
	void CheckNetCullDistance(UStaticMeshComponent* MeshComponent, FString& ErrorMessage);
	void CheckTrianglesLODNum(UStaticMesh* StaticMesh, FString& ErrorMessage);
	void CheckLODNumLimit(UStaticMesh* StaticMesh, FString& ErrorMessage);
	void CheckLODTrianglesLimit(UStaticMesh* StaticMesh, FString& ErrorMessage);
	void CheckLODScreenSizeLimit(UStaticMesh* StaticMesh, FString& ErrorMessage);
	void CheckLODUVChannelLimit(UStaticMesh* StaticMesh, FString& ErrorMessage);
	void CheckLODMaterialNumLimit(UStaticMesh* StaticMesh, FString& ErrorMessage);
	void CheckLODDuplicateMaterials(UStaticMesh* StaticMesh, FString& ErrorMessage);
	void CheckMeshMaterialNumLimit(UStaticMesh* StaticMesh, FString& ErrorMessage);

private:
	/** Property viewing widget */
	TSharedPtr<IDetailsView>   SettingsView;

	class UStaticMeshOptimizationRules* RuleSettings;
};

