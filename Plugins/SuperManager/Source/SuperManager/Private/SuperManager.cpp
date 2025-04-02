// Copyright Epic Games, Inc. All Rights Reserved.

#include "SuperManager.h"

#include "ContentBrowserModule.h"
#include "DebugHeader.h"
#include "ObjectTools.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "EditorAssetLibrary.h"

#define LOCTEXT_NAMESPACE "FSuperManagerModule"

void FSuperManagerModule::StartupModule()
{
	InitCBMenuExtention();
}

#pragma region 内容浏览器拓展

void FSuperManagerModule::InitCBMenuExtention()
{
	// 加载内容浏览器
	FContentBrowserModule& ContentBrowserModule=
	FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));

	// 获取内容浏览器右键菜单代理的引用
	TArray<FContentBrowserMenuExtender_SelectedPaths>& AssetContextMenuExtenders= ContentBrowserModule.GetAllPathViewContextMenuExtenders();

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
		const TArray<FString> FilePaths = UEditorAssetLibrary::ListAssets(FolderPathSelected,true,true);
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
			if ( UEditorAssetLibrary::DoesDirectoryHaveAssets(FilePath))
			{
				continue;
			}
			// 判断目录中是否有资产
			UEditorAssetLibrary::DeleteDirectory(FilePath);
		}
	}
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

void FSuperManagerModule::ShutdownModule()
{
	
}



#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FSuperManagerModule, SuperManager)