// Copyright Spitfire Interactive Pty Ltd. All Rights Reserved.


#include "a2fImportUI.h"

#include "a2fAssetImportData.h"
#include "AnimationUtils.h"


Ua2fImportUI::Ua2fImportUI()
{
	ResetToDefault();
}

void Ua2fImportUI::ResetToDefault()
{
	AnimSequenceImportData = CreateDefaultSubobject<Ua2fAssetImportData>(TEXT("AnimSequenceImportData"), true);

	BoneCompressionSettings = FAnimationUtils::GetDefaultAnimationBoneCompressionSettings();

	CurveCompressionSettings = FAnimationUtils::GetDefaultAnimationCurveCompressionSettings();
}

