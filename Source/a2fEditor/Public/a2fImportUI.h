// Copyright Spitfire Interactive Pty Ltd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "a2fImportUI.generated.h"

/**
 * 
 */

UCLASS(BlueprintType, HideCategories = Object, MinimalAPI)
class Ua2fImportUI : public UObject
{
public:
	Ua2fImportUI();

private:
	GENERATED_BODY()

public:
	void ResetToDefault();

	/** Skeleton to use for imported asset. When importing a mesh, leaving this as "None" will create a new skeleton. When importing an animation this MUST be specified to import the asset. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ImportSettings, meta = (ImportType = "SkeletalMesh|Animation"))
	class USkeleton* Skeleton;

	/** The bone compression settings used to compress bones in this sequence. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ImportSettings)
	class UAnimBoneCompressionSettings* BoneCompressionSettings;

	/** The curve compression settings used to compress curves in this sequence. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ImportSettings)
	class UAnimCurveCompressionSettings* CurveCompressionSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Transient, Instanced, Category = ImportSettings)
	class Ua2fAssetImportData* AnimSequenceImportData;


	bool bIsReimport;
};
