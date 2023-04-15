// Copyright Spitfire Interactive Pty Ltd. All Rights Reserved.

#include "a2fFactory.h"

#include "a2fAssetImportData.h"
#include "a2fImportUI.h"
#include "a2fOptionWindow.h"
#include "AnimationUtils.h"
#include "EditorFramework/AssetImportData.h"
#include "Interfaces/IMainFrameModule.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonSerializer.h"
#include "HAL/PlatformApplicationMisc.h"


Ua2fFactory::Ua2fFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bCreateNew = false;
	bEditorImport = true;
	bText = true;

	bShowOption = true;
	bOperationCanceled = false;

	ImportUI = nullptr;

	SupportedClass = UAnimSequence::StaticClass();
	Formats.Add("json;JavaScript Object Notation");
}

void Ua2fFactory::PostInitProperties()
{
	ImportUI = NewObject<Ua2fImportUI>(this, NAME_None, RF_NoFlags);

	Super::PostInitProperties();
}

bool Ua2fFactory::FactoryCanImport(const FString& Filename)
{
	return true;
}

inline UObject* Ua2fFactory::FactoryCreateText(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags,
	UObject* Context, const TCHAR* Type, const TCHAR*& Buffer, const TCHAR* BufferEnd, FFeedbackContext* Warn,
	bool& bOutOperationCanceled)
{

	//We are not re-importing
	ImportUI->bIsReimport = false;
	//ImportUI->ReimportMesh = nullptr;
	//ImportUI->bAllowContentTypeImport = true;

	// Show the import dialog only when not in a "yes to all" state or when automating import
	bool bIsAutomated = IsAutomatedImport();
	bool bShowImportDialog = bShowOption && !bIsAutomated;
	bool bImportAll = false;

	//Only try and read the old data on individual imports
	if(bShowImportDialog)
	{
		if (const UAnimSequence* ExistingAnimSequence = FindObject<UAnimSequence>(InParent, *InName.ToString()))
		{
			ImportUI->Skeleton = ExistingAnimSequence->GetSkeleton();
			ImportUI->BoneCompressionSettings = ExistingAnimSequence->BoneCompressionSettings;
			ImportUI->CurveCompressionSettings = ExistingAnimSequence->CurveCompressionSettings;

			if (Ua2fAssetImportData* ExistingImportData = Cast<Ua2fAssetImportData>(ExistingAnimSequence->AssetImportData))
			{
				ImportUI->AnimSequenceImportData = ExistingImportData;
			}
		}
	}
	


	if(bShowImportDialog)
	{
		TSharedPtr<SWindow> ParentWindow;

		if (FModuleManager::Get().IsModuleLoaded("MainFrame"))
		{
			IMainFrameModule& MainFrame = FModuleManager::LoadModuleChecked<IMainFrameModule>("MainFrame");
			ParentWindow = MainFrame.GetParentWindow();
		}

		// Compute centered window position based on max window size, which include when all categories are expanded
		const float ImportWindowWidth = 410.0f;
		const float ImportWindowHeight = 750.0f;
		FVector2D ImportWindowSize = FVector2D(ImportWindowWidth, ImportWindowHeight); // Max window size it can get based on current slate


		const FSlateRect WorkAreaRect = FSlateApplicationBase::Get().GetPreferredWorkArea();
		const FVector2D DisplayTopLeft(WorkAreaRect.Left, WorkAreaRect.Top);
		const FVector2D DisplaySize(WorkAreaRect.Right - WorkAreaRect.Left, WorkAreaRect.Bottom - WorkAreaRect.Top);

		const float ScaleFactor = FPlatformApplicationMisc::GetDPIScaleFactorAtPoint(DisplayTopLeft.X, DisplayTopLeft.Y);
		ImportWindowSize *= ScaleFactor;

		const FVector2D WindowPosition = (DisplayTopLeft + (DisplaySize - ImportWindowSize) / 2.0f) / ScaleFactor;

		TSharedRef<SWindow> Window = SNew(SWindow)
			.Title(NSLOCTEXT("UnrealEd", "a2fImportOpionsTitle", "a2f Import Options"))
			.SizingRule(ESizingRule::Autosized)
			.AutoCenter(EAutoCenter::None)
			.ClientSize(ImportWindowSize)
			.ScreenPosition(WindowPosition);

		TSharedPtr<Sa2fOptionWindow> A2FOptionWindow;
		Window->SetContent
		(
			SAssignNew(A2FOptionWindow, Sa2fOptionWindow)
			.ImportUI(ImportUI)
			.WidgetWindow(Window)
			.FullPath(FText::FromString(InParent->GetPathName()))
			.MaxWindowHeight(ImportWindowHeight)
			.MaxWindowWidth(ImportWindowWidth)
		);

		FSlateApplication::Get().AddModalWindow(Window, ParentWindow, false);

		bImportAll = A2FOptionWindow->ShouldImportAll();

		bOperationCanceled |= !A2FOptionWindow->ShouldImport();
	}

	if(bOperationCanceled)
	{
		bOutOperationCanceled = true;

		return nullptr;
	}

	if(bImportAll)
	{
		// If the user chose to import all, we don't show the dialog again and use the same settings for each object until importing another set of files
		bShowOption = false;
	}

	UAnimSequence* AnimSequence = CastChecked<UAnimSequence>(CreateOrOverwriteAsset(InClass, InParent, InName, Flags));

	if( AnimSequence == nullptr)
	{
		return nullptr;
	}

	USkeleton* Skeleton = ImportUI->Skeleton;

	AnimSequence->SetSkeleton(Skeleton);

	Ua2fAssetImportData* ImportData = Cast<Ua2fAssetImportData>(AnimSequence->AssetImportData);
	if (!ImportData)
	{
		ImportData = NewObject<Ua2fAssetImportData>(AnimSequence, NAME_None, RF_NoFlags, ImportUI->AnimSequenceImportData);

		// Try to preserve the source file data if possible
		if (AnimSequence->AssetImportData != nullptr)
		{
			ImportData->SourceData = AnimSequence->AssetImportData->SourceData;
		}

		AnimSequence->AssetImportData = ImportData;
	}

	AnimSequence->AssetImportData->AddFileName(UFactory::GetCurrentFilename(), 0);

	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
	const TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(Buffer);

	if (!(JsonObject.IsValid() && FJsonSerializer::Deserialize(JsonReader, JsonObject)))
	{
		return nullptr;
	}

	int32 NumPoses;
	JsonObject->TryGetNumberField(TEXT("numPoses"), NumPoses);
	int32 NumFrames;
	JsonObject->TryGetNumberField(TEXT("numFrames"), NumFrames);
	TArray<FString> CurveNames;
	JsonObject->TryGetStringArrayField(TEXT("facsNames"), CurveNames);

	const TArray<TSharedPtr<FJsonValue>>* WeightMat;
	JsonObject->TryGetArrayField(TEXT("weightMat"), WeightMat);

	const int32 FPS = ImportUI->AnimSequenceImportData->FrameRate;

	if (CurveNames.Num() == 0 || WeightMat == nullptr)
	{
		return nullptr;
	}

	bool bIsAdditiveAnim = ImportUI->AnimSequenceImportData->AdditiveAnimType == AAT_LocalSpaceBase || ImportUI->AnimSequenceImportData->AdditiveAnimType == AAT_RotationOffsetMeshSpace;

	const int32 Frames = WeightMat->Num();

	TSet<FString> CurvesToStrip;
	ImportUI->AnimSequenceImportData->GetCurvesToStrip(CurvesToStrip);

	for(int32 CurveIndex = 0; CurveIndex < CurveNames.Num(); ++CurveIndex)
	{
		const FString& CurveName = CurveNames[CurveIndex];

		if(CurvesToStrip.Contains(CurveName))
		{
			continue;
		}

		const USkeleton::AnimCurveUID CurveUID = Skeleton->GetUIDByName(USkeleton::AnimCurveMappingName, *CurveName);

		if (CurveUID == SmartName::MaxUID)
		{
			continue;
		}

		TArray<FRichCurveKey> Keys;

		Keys.Reserve(Frames);

		float WeightMin = FLT_MAX;
		float WeightMax = FLT_MIN;

		for (int32 FrameIndex = 0; FrameIndex < Frames; ++FrameIndex)
		{
			const TSharedPtr<FJsonValue>& WeightRow = (*WeightMat)[FrameIndex];

			const float Time = static_cast<float>(FrameIndex) / FPS;

			const TArray<TSharedPtr<FJsonValue>>& WeightValues = WeightRow->AsArray();

			const float Weight = WeightValues[CurveIndex]->AsNumber();

			WeightMin = FMath::Min(WeightMin, Weight);
			WeightMax = FMath::Max(WeightMax, Weight);

			Keys.Add(FRichCurveKey(Time, Weight));
		}

		//If the anim is additive and all the values are zero, don't bother adding a track 
		if(bIsAdditiveAnim && FMath::IsNearlyEqual(WeightMin,0.0f) && FMath::IsNearlyEqual(WeightMax, 0.0f))
		{
			continue;
		}

		FSmartName SmartName;
		Skeleton->GetSmartNameByUID(USkeleton::AnimCurveMappingName, CurveUID, SmartName);
		AnimSequence->RawCurveData.AddCurveData(SmartName);

		FFloatCurve* FloatCurveData = static_cast<FFloatCurve*>(AnimSequence->RawCurveData.GetCurveData(CurveUID));

		FloatCurveData->FloatCurve.SetKeys(Keys);
	}


	for (FCurveDrivenBoneTransform &CurveDrivenBoneTransform : ImportUI->AnimSequenceImportData->CurveDrivenBoneTransforms)
	{
		FRawAnimSequenceTrack RawTrack;
		RawTrack.PosKeys.Empty();
		RawTrack.RotKeys.Empty();
		RawTrack.ScaleKeys.Empty();

		TArray<int32> CurveIndices; 

		for (const FCurveDrivenTransform &CurveDrivenTransform : CurveDrivenBoneTransform.CurveDrivenTransforms)
		{
			CurveIndices.Add(CurveNames.Find(CurveDrivenTransform.Curve));
		}

		const FReferenceSkeleton &ReferenceSkeleton = Skeleton->GetReferenceSkeleton();
		const TArray<FTransform> &RawRefBonePose = ReferenceSkeleton.GetRawRefBonePose();

		int32 RawBoneIndex = ReferenceSkeleton.FindRawBoneIndex(CurveDrivenBoneTransform.Bone);

		FTransform RawBonePose = RawBoneIndex != INDEX_NONE ? RawRefBonePose[RawBoneIndex] : FTransform::Identity;

		for (int32 FrameIndex = 0; FrameIndex < Frames; ++FrameIndex)
		{
			FTransform LocalTransform;

			const TSharedPtr<FJsonValue>& WeightRow = (*WeightMat)[FrameIndex];
			const TArray<TSharedPtr<FJsonValue>>& WeightValues = WeightRow->AsArray();

			for (int32 CurveDrivenIndex = 0; CurveDrivenIndex < CurveDrivenBoneTransform.CurveDrivenTransforms.Num(); ++CurveDrivenIndex)
			{
				int32 CurveIndex = CurveIndices[CurveDrivenIndex];

				if(CurveIndex == INDEX_NONE)
				{
					continue;
				}

				FCurveDrivenTransform &CurveDrivenTransform = CurveDrivenBoneTransform.CurveDrivenTransforms[CurveDrivenIndex];

				FTransform::BlendFromIdentityAndAccumulate(LocalTransform, CurveDrivenTransform.Transform, ScalarRegister(WeightValues[CurveIndex]->AsNumber()));
			}

			FTransform CombinedTransform = RawBonePose* LocalTransform;

			RawTrack.ScaleKeys.Add(CombinedTransform.GetScale3D());
			RawTrack.PosKeys.Add(CombinedTransform.GetTranslation());
			RawTrack.RotKeys.Add(CombinedTransform.GetRotation());
		}

		AnimSequence->AddNewRawTrack(CurveDrivenBoneTransform.Bone, &RawTrack);
	}


	AnimSequence->SetRawNumberOfFrame(Frames);
	AnimSequence->SequenceLength = static_cast<float>(FMath::Max(Frames - 1, 1)) / FPS;
	AnimSequence->ImportFileFramerate = FPS;

	AnimSequence->BoneCompressionSettings = ImportUI->BoneCompressionSettings;
	AnimSequence->CurveCompressionSettings = ImportUI->CurveCompressionSettings;

	AnimSequence->AdditiveAnimType = ImportUI->AnimSequenceImportData->AdditiveAnimType;
	
	return AnimSequence;
}

void Ua2fFactory::CleanUp()
{
	bShowOption = true;
	bOperationCanceled = false;

	Super::CleanUp();
}

int32 Ua2fFactory::GetPriority() const
{
	return INT32_MAX;
}

bool Ua2fFactory::CanReimport(UObject* Obj, TArray<FString>& OutFilenames)
{
	if(const UAnimSequence* AnimSequence = Cast<UAnimSequence>(Obj))
	{
		if(const UAssetImportData* AssetImportData = AnimSequence->AssetImportData)
		{
			AssetImportData->ExtractFilenames(OutFilenames);
			return true;
		}
	}

	return false;
}

void Ua2fFactory::SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths)
{
	if (const UAnimSequence* AnimSequence = Cast<UAnimSequence>(Obj))
	{
		if(NewReimportPaths.Num() == 1)
		{
			if(UAssetImportData* AssetImportData = AnimSequence->AssetImportData)
			{
				AssetImportData->UpdateFilenameOnly(NewReimportPaths[0]);
			}
		}
	}
}

EReimportResult::Type Ua2fFactory::Reimport(UObject* Obj)
{
	const UAnimSequence* AnimSequence = Cast<UAnimSequence>(Obj);

	if (AnimSequence == nullptr)
	{
		return EReimportResult::Failed;
	}

	const FString ResolvedSourceFilePath = AnimSequence->AssetImportData->GetFirstFilename();

	if (ResolvedSourceFilePath.IsEmpty())
	{
		return EReimportResult::Failed;
	}

	if (IFileManager::Get().FileSize(*ResolvedSourceFilePath) == INDEX_NONE)
	{
		return EReimportResult::Failed;
	}

	bool bOutCanceled = false;
	if (ImportObject(AnimSequence->GetClass(), AnimSequence->GetOuter(), *AnimSequence->GetName(), RF_Public | RF_Standalone, ResolvedSourceFilePath, nullptr, bOutCanceled))
	{
		return EReimportResult::Succeeded;
	}

	if (bOutCanceled)
	{
		return EReimportResult::Cancelled;
	}

	return EReimportResult::Failed;
}
