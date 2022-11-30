#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SStaticMeshOptimizationPage : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SStaticMeshOptimizationPage) { }
	SLATE_END_ARGS()

public:
	typedef TSharedPtr<class FEditorStaticMesh> FEditorStaticMeshPtr;
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
	void CheckTrianglesLODNum(FString& ErrorMessage);
	void CheckLODNumLimit(FString& ErrorMessage);
	void CheckLODTrianglesLimit(FString& ErrorMessage);
	void CheckLODScreenSizeLimit(FString& ErrorMessage);
	void CheckLODUVChannelLimit(FString& ErrorMessage);
	void CheckLODMaterialNumLimit(FString& ErrorMessage);
	void CheckLODDuplicateMaterials(FString& ErrorMessage);
	void CheckMeshMaterialNumLimit(FString& ErrorMessage);
	void DumpSortedMeshTriangles(const TArray<UStaticMesh*>& Meshes);


private:
	/** Property viewing widget */
	TSharedPtr<IDetailsView>   SettingsView;
	FEditorStaticMeshPtr EditorStaticMesh;
	class UStaticMeshOptimizationRules* RuleSettings;
};

