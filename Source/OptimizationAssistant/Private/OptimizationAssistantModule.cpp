// Copyright Epic Games, Inc. All Rights Reserved.

#include "OptimizationAssistantModule.h"
#include "OptimizationAssistantStyle.h"
#include "OptimizationAssistantCommands.h"
#include "LevelEditor.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "ToolMenus.h"
#include "ISettingsModule.h"
#include "ISettingsSection.h"
#include "Interfaces/IPluginManager.h"

#include "Widgets/SOptimizationAssistantView.h"
#include "Widgets/StaticMesh/StaticMeshOptimizationRules.h"
#include "Widgets/SkeletalMesh/SkeletalMeshOptimizationRules.h"


static const FName OptimizationAssistantTabName("OptimizationAssistant");

#define LOCTEXT_NAMESPACE "OptimizationAssistantPlugin"

FString FOptimizationAssistantModule::OptimizationAssistantIni;

void FOptimizationAssistantModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	
	FOptimizationAssistantStyle::Initialize();
	FOptimizationAssistantStyle::ReloadTextures();
	FOptimizationAssistantCommands::Register();

	FConfigCacheIni::LoadGlobalIniFile(OptimizationAssistantIni, TEXT("OptimizationAssistant"));

	PluginCommands = MakeShareable(new FUICommandList);
	PluginCommands->MapAction(
		FOptimizationAssistantCommands::Get().OpenPluginWindow,
		FExecuteAction::CreateRaw(this, &FOptimizationAssistantModule::PluginButtonClicked),
		FCanExecuteAction());

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FOptimizationAssistantModule::RegisterMenus));
	
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(OptimizationAssistantTabName, FOnSpawnTab::CreateRaw(this, &FOptimizationAssistantModule::OnSpawnPluginTab))
		.SetDisplayName(LOCTEXT("FOptimizationAssistantTabTitle", "OptimizationAssistant"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);

	// register settings

	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		ISettingsSectionPtr StaticMeshSettingsSection = SettingsModule->RegisterSettings("Project", "Plugins", "OptimizationAssistant",
			LOCTEXT("StaticMeshOptimizationRules", "StaticMeshOptimizationRules"),
			LOCTEXT("OptimizationAssistantDescription", "Configure optimization rules."),
			GetMutableDefault<UStaticMeshOptimizationRules>()
		);

		ISettingsSectionPtr SkeletalMeshSettingsSection = SettingsModule->RegisterSettings("Project", "Plugins", "OptimizationAssistant",
			LOCTEXT("SkeletalMeshOptimizationRules", "SkeletalMeshOptimizationRules"),
			LOCTEXT("OptimizationAssistantDescription", "Configure optimization rules."),
			GetMutableDefault<USkeletalMeshOptimizationRules>()
		);

		if (StaticMeshSettingsSection.IsValid())
		{
			StaticMeshSettingsSection->OnModified().BindRaw(this, &FOptimizationAssistantModule::HandleSettingsSaved);
		}

		if (SkeletalMeshSettingsSection.IsValid())
		{
			SkeletalMeshSettingsSection->OnModified().BindRaw(this, &FOptimizationAssistantModule::HandleSettingsSaved);
		}
	}
}

void FOptimizationAssistantModule::ShutdownModule()
{
	GConfig->Flush(false, FOptimizationAssistantModule::OptimizationAssistantIni);

	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->UnregisterSettings("Project", "Plugins", "OptimizationAssistant");
	}

	UToolMenus::UnRegisterStartupCallback(this);

	UToolMenus::UnregisterOwner(this);

	FOptimizationAssistantStyle::Shutdown();

	FOptimizationAssistantCommands::Unregister();

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(OptimizationAssistantTabName);
}

TSharedRef<SDockTab> FOptimizationAssistantModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SOptimizationAssistantView)
		];
}

void FOptimizationAssistantModule::PluginButtonClicked()
{
	FGlobalTabmanager::Get()->TryInvokeTab(OptimizationAssistantTabName);
}


bool FOptimizationAssistantModule::HandleSettingsSaved()
{
	//UStaticMeshOptimizationRules* Settings = GetMutableDefault<UStaticMeshOptimizationRules>();
	//Settings->SaveConfig();
	//GConfig->Flush(false, OptimizationAssistantIni);
	return true;
}

void FOptimizationAssistantModule::RegisterMenus()
{
	// Owner will be used for cleanup in call to UToolMenus::UnregisterOwner
	FToolMenuOwnerScoped OwnerScoped(this);

	{
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
		{
			FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");
			Section.AddMenuEntryWithCommandList(FOptimizationAssistantCommands::Get().OpenPluginWindow, PluginCommands);
		}
	}

	{
		//UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar");
		//{
		//	FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("Settings");
		//	{
		//		FToolMenuEntry& Entry = Section.AddEntry(FToolMenuEntry::InitToolBarButton(FOptimizationAssistantCommands::Get().OpenPluginWindow));
		//		Entry.SetCommandList(PluginCommands);
		//	}
		//}
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FOptimizationAssistantModule, OptimizationAssistant)