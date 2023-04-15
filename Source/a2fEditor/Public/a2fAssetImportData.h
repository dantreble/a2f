// Copyright Spitfire Interactive Pty Ltd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EditorFramework/AssetImportData.h"
#include "a2fAssetImportData.generated.h"

/**
 * 
 */

USTRUCT(BlueprintType)
struct FCurveDrivenTransform
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Curve;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool StripCurveTrack = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FTransform Transform;
};

USTRUCT(BlueprintType)
struct FCurveDrivenBoneTransform
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName Bone;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FCurveDrivenTransform> CurveDrivenTransforms;
};

UCLASS(BlueprintType)
class A2FEDITOR_API Ua2fAssetImportData : public UAssetImportData
{
	GENERATED_BODY()

public:

	UFUNCTION()
	void GetCurvesToStrip(TSet<FString>& CurvesToStrip) const;

	/** Use this option to specify a sample rate for the imported animation*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ToolTip = "Animation frames per second", ClampMin = 0, UIMin = 0, ClampMax = 48000, UIMax = 60))
	int32 FrameRate = 30;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TEnumAsByte<enum EAdditiveAnimationType> AdditiveAnimType = AAT_LocalSpaceBase;

	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	TArray<FCurveDrivenBoneTransform> CurveDrivenBoneTransforms;

};
