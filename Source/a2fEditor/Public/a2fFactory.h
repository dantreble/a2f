// Copyright Spitfire Interactive Pty Ltd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "EditorReimportHandler.h"
#include "a2fFactory.generated.h"

/**
 * 
 */
UCLASS()
class A2FEDITOR_API Ua2fFactory : public UFactory, public FReimportHandler
{
	GENERATED_UCLASS_BODY()

public:

	Ua2fFactory();

	virtual void PostInitProperties() override;

	virtual bool FactoryCanImport(const FString& Filename) override;

	virtual UObject* FactoryCreateText(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags,
		UObject* Context, const TCHAR* Type, const TCHAR*& Buffer, const TCHAR* BufferEnd, FFeedbackContext* Warn,
		bool& bOutOperationCanceled) override;
	virtual void CleanUp() override;


	// FReimportHandler
	virtual int32 GetPriority() const override;
	virtual bool CanReimport(UObject* Obj, TArray<FString>& OutFilenames) override;
	virtual void SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths) override;
	virtual EReimportResult::Type Reimport(UObject* Obj) override;


private:

	UPROPERTY()
	class Ua2fImportUI* ImportUI;


	bool bShowOption;

	/** true if the import operation was canceled. */
	bool bOperationCanceled;
};


