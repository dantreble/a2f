// Copyright Spitfire Interactive Pty Ltd. All Rights Reserved.


#include "a2fAssetImportData.h"


void Ua2fAssetImportData::GetCurvesToStrip(TSet<FString> &CurvesToStrip) const
{
	for (const FCurveDrivenBoneTransform &CurveDrivenBoneTransform : CurveDrivenBoneTransforms)
	{
		for (const FCurveDrivenTransform &CurveDrivenTransform : CurveDrivenBoneTransform.CurveDrivenTransforms)
		{
			if(CurveDrivenTransform.StripCurveTrack)
			{
				CurvesToStrip.Add(CurveDrivenTransform.Curve);
			}
		}
	}
}
