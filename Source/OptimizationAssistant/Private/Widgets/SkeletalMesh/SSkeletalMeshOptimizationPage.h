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
    int32 GetMeshMaxTriangles(USkeletalMesh* SkeletalMesh);
	void ProcessOptimizationCheck(USkeletalMeshComponent* MeshComponent, FOutputDevice& Ar);
	void ProcessOptimizationCheck(USkeletalMesh* SkeletalMesh, FOutputDevice& Ar);
	void ProcessOptimizationCheck(UAnimSequence* AnimSequence, FOutputDevice& Ar);

	void CheckCullDistance(USkeletalMeshComponent* MeshComponent, FString& ErrorMessage);
	void CheckNetCullDistance(USkeletalMeshComponent* MeshComponent, FString& ErrorMessage);
	void CheckTrianglesLODNum(USkeletalMesh* SkeletalMesh, FString& ErrorMessage);
	void CheckLODNumLimit(USkeletalMesh* SkeletalMesh, FString& ErrorMessage);
	void CheckLODTrianglesLimit(USkeletalMesh* SkeletalMesh, FString& ErrorMessage);
	void CheckLODScreenSizeLimit(USkeletalMesh* SkeletalMesh, FString& ErrorMessage);
	void CheckLODUVChannelLimit(USkeletalMesh* SkeletalMesh, FString& ErrorMessage);
	void CheckLODMaterialNumLimit(USkeletalMesh* SkeletalMesh, FString& ErrorMessage);
	void CheckLODDuplicateMaterials(USkeletalMesh* SkeletalMesh, FString& ErrorMessage);
	void CheckMeshMaterialNumLimit(USkeletalMesh* SkeletalMesh, FString& ErrorMessage);

private:
	/** Property viewing widget */
	TSharedPtr<IDetailsView>   SettingsView;

	class USkeletalMeshOptimizationRules* RuleSettings;
};

