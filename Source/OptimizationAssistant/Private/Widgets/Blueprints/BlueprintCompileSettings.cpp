#include "BlueprintCompileSettings.h"

UBlueprintCompileSettings::UBlueprintCompileSettings()
	: Super()
{
	bResultsOnly = false;
	bDirtyOnly = false;
	IterativeCompiling = true;
	bSimpleAssetList = false;
}

