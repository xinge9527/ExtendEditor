// Copyright Epic Games, Inc. All Rights Reserved.

#include "SuperManager.h"

#include "ContentBrowserModule.h"
#include "DebugHeader.h"
#include "ObjectTools.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "EditorAssetLibrary.h"
#include "SlateWidgets/AdvanceDeletionWidget.h"

#define LOCTEXT_NAMESPACE "FSuperManagerModule"

void FSuperManagerModule::StartupModule()
{
	InitCBMenuExtention();

	RegisterAdvanceDeletionTab();
}

#pragma region 内容浏览器拓展

void FSuperManagerModule::InitCBMenuExtention()
{
	// 加载内容浏览器
	FContentBrowserModule& ContentBrowserModule =
		FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));

	// 获取内容浏览器右键菜单代理的引用
	TArray<FContentBrowserMenuExtender_SelectedPaths>& AssetContextMenuExtenders = ContentBrowserModule.GetAllPathViewContextMenuExtenders();

	// 将自己的代码增加到代理里面，就能新增一个右键按钮
	FContentBrowserMenuExtender_SelectedPaths CustomCBMenuDelegate;
	// 将自己的右键按钮(MenuExtender)绑定进去
	// CustomCBMenuDelegate.BindRaw(this,&FSuperManagerModule::CustomCBMenuExtender);
	// AssetContextMenuExtenders.Add(CustomCBMenuDelegate);
	//与👆的写法一个意思，只是更加简洁
	AssetContextMenuExtenders.Add(
		FContentBrowserMenuExtender_SelectedPaths::CreateRaw(
			this, &FSuperManagerModule::CustomCBMenuExtender
		)
	);
}

TSharedRef<FExtender> FSuperManagerModule::CustomCBMenuExtender(const TArray<FString>& SelectedPaths)
{
	TSharedRef<FExtender> MenuExtender(new FExtender());

	// 当选中的文件大于的时候
	if (SelectedPaths.Num() > 0)
	{
		// 朝右键菜单插入一个自己的按钮
		MenuExtender->AddMenuExtension(
			FName("Delete"),
			EExtensionHook::After,
			TSharedPtr<FUICommandList>(), // 如果想增加自定义快捷键，可以在此处实现
			FMenuExtensionDelegate::CreateRaw(this, &FSuperManagerModule::AddCBMenuEntry));

		FolderPathsSelected = SelectedPaths;
	}

	return MenuExtender;
}

void FSuperManagerModule::AddCBMenuEntry(FMenuBuilder& MenuBuilder)
{
	MenuBuilder.AddMenuEntry(
		FText::FromString(TEXT("删除未被使用的资产")),
		FText::FromString(TEXT("安全的删除未被使用的资产")),
		FSlateIcon(),
		FExecuteAction::CreateRaw(this, &FSuperManagerModule::OnDeleteUnusedAssetButtonClicked)
	);

	MenuBuilder.AddMenuEntry(
		FText::FromString(TEXT("删除空文件目录")),
		FText::FromString(TEXT("删除空文件目录")),
		FSlateIcon(),
		FExecuteAction::CreateRaw(this, &FSuperManagerModule::OnDeleteEmptyFolders)
	);

	MenuBuilder.AddMenuEntry(
		FText::FromString(TEXT("高级删除")),
		FText::FromString(TEXT("List assets by specific condition in a tab for deleting")),
		FSlateIcon(),
		FExecuteAction::CreateRaw(this, &FSuperManagerModule::AdvanceDeletionButtonClicked)
	);
}

void FSuperManagerModule::OnDeleteUnusedAssetButtonClicked()
{
	if (FolderPathsSelected.Num() == 0)
	{
		return;
	}
	if (FolderPathsSelected.Num() > 1)
	{
		DebugHeader::ShowMesDialog(EAppMsgType::Ok,TEXT("请不要同时选取多个文件"));
		return;
	}

	const TArray<FString> AssetsPathNames = UEditorAssetLibrary::ListAssets(FolderPathsSelected[0]);

	if (AssetsPathNames.Num() == 0)
	{
		DebugHeader::ShowMesDialog(EAppMsgType::Ok,TEXT("当前目录下未查找到资产"));
		return;
	}

	EAppReturnType::Type ConfirmResult = DebugHeader::ShowMesDialog(EAppMsgType::YesNo,TEXT("当前文件夹下有") + FString::FromInt(AssetsPathNames.Num()) + TEXT("个文件，是否确认删除？"));
	DebugHeader::Print(TEXT("当前选中的文件夹：") + FolderPathsSelected[0]);

	if (ConfirmResult == EAppReturnType::No)
	{
		return;
	}

	FixUpRedirectors();

	TArray<FAssetData> UnusedAssetDataArray;
	for (const FString& AssetPathName : AssetsPathNames)
	{
		// 过滤掉不需要检索的文件
		if (AssetPathName.Contains(TEXT("Developers")) ||
			AssetPathName.Contains(TEXT("Collections"))
		)
		{
			continue;
		}

		if (!UEditorAssetLibrary::DoesAssetExist(AssetPathName))continue;

		TArray<FString> AssetReferences = UEditorAssetLibrary::FindPackageReferencersForAsset(AssetPathName);

		if (AssetReferences.Num() == 0)
		{
			const FAssetData UnusedAssetData = UEditorAssetLibrary::FindAssetData(AssetPathName);
			UnusedAssetDataArray.Add(UnusedAssetData);
		}
	}
	if (UnusedAssetDataArray.Num() > 0)
	{
		ObjectTools::DeleteAssets(UnusedAssetDataArray);
	}
	else
	{
		DebugHeader::ShowMesDialog(EAppMsgType::Ok,TEXT("当前文件夹下没有未被引用的资产"));
	}
}

void FSuperManagerModule::OnDeleteEmptyFolders()
{
	if (FolderPathsSelected.Num() == 0)
	{
		return;
	}

	for (FString FolderPathSelected : FolderPathsSelected)
	{
		const TArray<FString> FilePaths = UEditorAssetLibrary::ListAssets(FolderPathSelected, true, true);
		for (FString FilePath : FilePaths)
		{
			if (FilePath.Contains(TEXT("Developers")) ||
				FilePath.Contains(TEXT("Collections")))
			{
				continue;
			}
			if (!UEditorAssetLibrary::DoesDirectoryExist(FilePath))
			{
				continue;
			}
			if (UEditorAssetLibrary::DoesDirectoryHaveAssets(FilePath))
			{
				continue;
			}
			// 判断目录中是否有资产
			UEditorAssetLibrary::DeleteDirectory(FilePath);
		}
	}
}

void FSuperManagerModule::AdvanceDeletionButtonClicked()
{
	FixUpRedirectors();

	FGlobalTabmanager::Get()->TryInvokeTab(FName("AdvanceDeletion"));
}

void FSuperManagerModule::FixUpRedirectors()
{
	TArray<UObjectRedirector*> RedirectorToFixArray;

	const FAssetRegistryModule& AssetRegistryModule =
		FModuleManager::Get().LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

	FARFilter Filter;
	Filter.bRecursivePaths = true;
	Filter.PackagePaths.Emplace("/Game");
	Filter.ClassPaths.Emplace("/Script/CoreUObject.UObjectRedirector");

	TArray<FAssetData> OutRedirectorArray;
	AssetRegistryModule.Get().GetAssets(Filter, OutRedirectorArray);

	for (const FAssetData& RedirectorData : OutRedirectorArray)
	{
		if (UObjectRedirector* RedirectorToFix = Cast<UObjectRedirector>(RedirectorData.GetAsset()))
		{
			RedirectorToFixArray.Add(RedirectorToFix);
		}
	}

	const FAssetToolsModule& AssetToolsModule =
		FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));

	AssetToolsModule.Get().FixupReferencers(RedirectorToFixArray);
}

#pragma endregion

#pragma region CustomEditorTab

void FSuperManagerModule::RegisterAdvanceDeletionTab()
{
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(FName("AdvanceDeletion"),
	                                                  FOnSpawnTab::CreateRaw(this, &FSuperManagerModule::OnSpawnAdvanceDeletionTab))
	                        .SetDisplayName(FText::FromString(TEXT("Advance Deletion")));
}

TSharedRef<SDockTab> FSuperManagerModule::OnSpawnAdvanceDeletionTab(const FSpawnTabArgs& SpawnTabArgs)
{
	return SNew(SDockTab).TabRole(ETabRole::NomadTab)
		[
			SNew(SAdvanceDeletionTab)
			.AssetDataToStore(GetAllAssetDataUnderSelectedFolder())
			.CurrentSelectedFolder(FolderPathsSelected[0])
		];
}

TArray<TSharedPtr<FAssetData>> FSuperManagerModule::GetAllAssetDataUnderSelectedFolder()
{
	if (FolderPathsSelected.IsEmpty())
	{
		return TArray<TSharedPtr<FAssetData>>();
	}
	TArray<TSharedPtr<FAssetData>> AssetDataArray;
	TArray<FString> AssetsPathNames = UEditorAssetLibrary::ListAssets(FolderPathsSelected[0], true, true);

	TArray<FAssetData> UnusedAssetDataArray;
	for (const FString& AssetPathName : AssetsPathNames)
	{
		// 过滤掉不需要检索的文件
		if (AssetPathName.Contains(TEXT("Developers")) ||
			AssetPathName.Contains(TEXT("Collections"))
		)
		{
			continue;
		}

		if (!UEditorAssetLibrary::DoesAssetExist(AssetPathName))continue;

		const FAssetData Data = UEditorAssetLibrary::FindAssetData(AssetPathName);
		AssetDataArray.Add(MakeShared<FAssetData>(Data));
	}
	return AssetDataArray;
}

#pragma endregion

#pragma region ProcessDataForAssetList

bool FSuperManagerModule::DeleteSingleAssetForAssetList(const FAssetData& AssetDataToDelete)
{
	TArray<FAssetData> AssetDataForDeletion;
	AssetDataForDeletion.Add(AssetDataToDelete);
	if (ObjectTools::DeleteAssets(AssetDataForDeletion) > 0)
	{
		return true;
	}
	return false;
}

bool FSuperManagerModule::DeleteMultipleAssetForAssetList(const TArray<FAssetData>& AssetsToDelete)
{
	if (ObjectTools::DeleteAssets(AssetsToDelete) > 0)
	{
		return true;
	}
	return false;
}

void FSuperManagerModule::ListUnusedAssetsForAssetList(const TArray<TSharedPtr<FAssetData>>& AssetDataToFilter, TArray<TSharedPtr<FAssetData>>& OutUnusedAssetData)
{
	OutUnusedAssetData.Empty();
	for (const TSharedPtr<FAssetData>& DataSharedPtr : AssetDataToFilter)
	{
		TArray<FString> AssetReferences =
			UEditorAssetLibrary::FindPackageReferencersForAsset(DataSharedPtr->GetObjectPathString());
		if (AssetReferences.Num() == 0)
		{
			OutUnusedAssetData.Add(DataSharedPtr);
		}
	}
}

void FSuperManagerModule::ListSameNameAsssetsForAssetList(const TArray<TSharedPtr<FAssetData>>& AssetsToFilter, TArray<TSharedPtr<FAssetData>>& OutSameNameAssetData)
{
	OutSameNameAssetData.Empty();

	TMultiMap<FString, TSharedPtr<FAssetData>> AssetInfoMultiMap;
	for (const TSharedPtr<FAssetData>& DataSharedPtr : AssetsToFilter)
	{
		AssetInfoMultiMap.Emplace(DataSharedPtr->AssetName.ToString(), DataSharedPtr);
	}
	for (const TSharedPtr<FAssetData>& DataSharedPtr : AssetsToFilter)
	{
		TArray<TSharedPtr<FAssetData>> OutAssetsData;
		AssetInfoMultiMap.MultiFind(DataSharedPtr->AssetName.ToString(), OutAssetsData);
		if (OutAssetsData.Num() <= 1)continue;
		for (TSharedPtr<FAssetData> SameNameData : OutAssetsData)
		{
			if (SameNameData.IsValid())
			{
				OutSameNameAssetData.AddUnique(SameNameData);
			}
		}
	}
}

void FSuperManagerModule::SyncCBToClickedAssetForAssetList(const FString& AssetPathToSync)
{
	TArray<FString> AssetsPathToSync;
	AssetsPathToSync.Add(AssetPathToSync);
	UEditorAssetLibrary::SyncBrowserToObjects(AssetsPathToSync);
}

#pragma endregion

void FSuperManagerModule::ShutdownModule()
{
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(FName("AdvanceDeletion"));
}


#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FSuperManagerModule, SuperManager)
