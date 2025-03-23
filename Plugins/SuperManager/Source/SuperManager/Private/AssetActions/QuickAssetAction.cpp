// Fill out your copyright notice in the Description page of Project Settings.


#include "AssetActions/QuickAssetAction.h"

#include "AssetToolsModule.h"
#include "DebugHeader.h"
#include "EditorUtilityLibrary.h"
#include "EditorAssetLibrary.h"
#include "ObjectTools.h"
#include "AssetRegistry/AssetRegistryModule.h"

void UQuickAssetAction::DuplicateAssets(int32 NumOfDuplicates)
{
	if (NumOfDuplicates <= 0)
	{
		DebugHeader::ShowMesDialog(EAppMsgType::Ok, "请输入大于0的整数", true);
		return;
	}

	TArray<FAssetData> SelectedAssetDataArray = UEditorUtilityLibrary::GetSelectedAssetData();
	uint32 Counter = 0;

	for (const FAssetData& SelectedAssetData : SelectedAssetDataArray)
	{
		for (int32 i = 0; i < NumOfDuplicates; i++)
		{
			const FString SourceAssetPath = SelectedAssetData.GetObjectPathString();
			const FString NewDuplicatedAssetName = SelectedAssetData.AssetName.ToString() + "_" + FString::FromInt(i + 1);
			const FString NewPathName = FPaths::Combine(SelectedAssetData.PackagePath.ToString(), NewDuplicatedAssetName);

			if (UEditorAssetLibrary::DuplicateAsset(SourceAssetPath, NewPathName))
			{
				UEditorAssetLibrary::SaveAsset(NewPathName, false);
				++Counter;
			}
		}
	}

	if (Counter > 0)
	{
		DebugHeader::ShowNotifyInfo("Successfully duplicated " + FString::FromInt(Counter) + " files");
	}
}

void UQuickAssetAction::AddPrefixes()
{
	TArray<UObject*> SelectedObjectArray = UEditorUtilityLibrary::GetSelectedAssets();
	uint32 Counter = 0;

	for (UObject* SelectedObject : SelectedObjectArray)
	{
		if (!SelectedObject)
		{
			continue;
		}

		FString* Prefix = PrefixMap.Find(SelectedObject->GetClass());
		if (!Prefix || Prefix->IsEmpty())
		{
			DebugHeader::Print(TEXT("查找class的前缀失败:") + SelectedObject->GetClass()->GetName());
			continue;
		}
		FString OldName = SelectedObject->GetName();
		if (OldName.StartsWith(*Prefix))
		{
			DebugHeader::Print(OldName + TEXT(" 已经存在前缀名！"));
			continue;
		}
		if (SelectedObject->IsA<UMaterialInstanceConstant>())
		{
			OldName.RemoveFromStart(TEXT("M_"));
			OldName.RemoveFromEnd(TEXT("_Inst"));
		}
		FString NewName = *Prefix + OldName;

		UEditorUtilityLibrary::RenameAsset(SelectedObject, NewName);
		++Counter;
	}
	if (Counter > 0)
	{
		DebugHeader::ShowNotifyInfo(TEXT("成功修改 " + FString::FromInt(Counter) + TEXT(" 个文件前缀名")));
	}
}

void UQuickAssetAction::RemoveUnusedAssets()
{
	TArray<FAssetData> SelectedAssetDataArray = UEditorUtilityLibrary::GetSelectedAssetData();
	TArray<FAssetData> UnusedAssetsDataArray;

	FixUpRedirectors();
	
	for (FAssetData SelectedAssetData : SelectedAssetDataArray)
	{
		TArray<FString> AssetReferencerArray =
			UEditorAssetLibrary::FindPackageReferencersForAsset(SelectedAssetData.GetObjectPathString());

		if (AssetReferencerArray.Num() == 0)
		{
			UnusedAssetsDataArray.Add(SelectedAssetData);
		}
	}
	if (UnusedAssetsDataArray.Num() == 0)
	{
		DebugHeader::ShowMesDialog(EAppMsgType::Ok,TEXT("在选中的资产中，没有无引用的资产"), true);
		return;
	}

	int32 NumOfAssetsDeleted = ObjectTools::DeleteAssets(UnusedAssetsDataArray);
	if (NumOfAssetsDeleted > 0)
	{
		DebugHeader::ShowNotifyInfo(TEXT("成功删除 ") + FString::FromInt(NumOfAssetsDeleted) + TEXT(" 个未被引用的资产"));
	}
}

void UQuickAssetAction::FixUpRedirectors()
{
	TArray<UObjectRedirector*> RedirectorToFixArray;

	FAssetRegistryModule& AssetRegistryModule =
		FModuleManager::Get().LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

	FARFilter Filter;
	Filter.bRecursivePaths = true;
	Filter.PackagePaths.Emplace("/Game");
	Filter.ClassPaths.Emplace("ObjectRedirector");

	TArray<FAssetData> OutRedirectorArray;
	AssetRegistryModule.Get().GetAssets(Filter, OutRedirectorArray);

	for (const FAssetData& RedirectorData : OutRedirectorArray)
	{
		if (UObjectRedirector* RedirectorToFix = Cast<UObjectRedirector>(RedirectorData.GetAsset()))
		{
			RedirectorToFixArray.Add(RedirectorToFix);
		}
	}

	FAssetToolsModule& AssetToolsModule =
		FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));

	AssetToolsModule.Get().FixupReferencers(RedirectorToFixArray);
}
