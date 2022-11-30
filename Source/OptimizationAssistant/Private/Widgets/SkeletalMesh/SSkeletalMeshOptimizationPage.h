#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SSkeletalMeshOptimizationPage : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SSkeletalMeshOptimizationPage) { }
	SLATE_END_ARGS()

public:
	typedef TSharedPtr<class FEditorSkeletalMesh> FEditorSkeletalMeshPtr;
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
    int32 GetMeshMaxTriangles(USkeletalMesh* SkeletalMesh);
	void ProcessOptimizationCheck(USkeletalMeshComponent* MeshComponent, FOutputDevice& Ar);
	void ProcessOptimizationCheck(USkeletalMesh* SkeletalMesh, FOutputDevice& Ar);
	void ProcessOptimizationCheck(UAnimSequence* AnimSequence, FOutputDevice& Ar);

	void CheckCullDistance(USkeletalMeshComponent* MeshComponent, FString& ErrorMessage);
	void CheckNetCullDistance(USkeletalMeshComponent* MeshComponent, FString& ErrorMessage);
	void CheckTrianglesLODNum(FString& ErrorMessage);
	void CheckLODNumLimit(FString& ErrorMessage);
	void CheckLODTrianglesLimit(FString& ErrorMessage);
	void CheckLODScreenSizeLimit(FString& ErrorMessage);
	void CheckLODUVChannelLimit(FString& ErrorMessage);
	void CheckLODMaterialNumLimit(FString& ErrorMessage);
	void CheckLODDuplicateMaterials(FString& ErrorMessage);
	void CheckMeshMaterialNumLimit(FString& ErrorMessage);
	void DumpSortedMeshTriangles(const TArray<USkeletalMesh*>& Meshes);

private:
	/** Property viewing widget */
	TSharedPtr<IDetailsView>   SettingsView;
	FEditorSkeletalMeshPtr EditorSkeletalMesh;
	class USkeletalMeshOptimizationRules* RuleSettings;
};

