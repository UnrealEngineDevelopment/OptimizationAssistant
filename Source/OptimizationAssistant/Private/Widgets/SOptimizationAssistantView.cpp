
#include "SOptimizationAssistantView.h"
#include "SlateOptMacros.h"
#include "EditorFontGlyphs.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SGridPanel.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Misc/ScopedSlowTask.h"
#include "Misc/OutputDeviceArchiveWrapper.h"
#include "Interfaces/IPluginManager.h"
#include "OptimizationAssistantModule.h"
#include "SGlobalSettingsPage.h"
#include "PlatformInfo.h"

#include "StaticMesh/SStaticMeshOptimizationPage.h"
#include "StaticMesh/StaticMeshOptimizationRules.h"

#include "SkeletalMesh/SSkeletalMeshOptimizationPage.h"
#include "SkeletalMesh/SkeletalMeshOptimizationRules.h"

#include "ParticleSystem/SParticleSystemOptimizationPage.h"
#include "ParticleSystem/ParticleSystemOptimizationRules.h"

#include "Blueprints/SBlueprintCompilePage.h"
#include "Blueprints/BlueprintCompileSettings.h"


#define LOCTEXT_NAMESPACE "OptimizationAssistantPlugin"

SOptimizationAssistantView::SOptimizationAssistantView()
{

}

SOptimizationAssistantView::~SOptimizationAssistantView()
{
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SOptimizationAssistantView::Construct(const FArguments & InArgs)
{
	EnableStaticMeshCheck = ECheckBoxState::Checked;
	EnableSkeletalMeshCheck = ECheckBoxState::Checked;
	EnableParticleSystemCheck = ECheckBoxState::Checked;
	EnableBlueprintCompileCheck = ECheckBoxState::Unchecked;

	StaticMeshOptimizationPage = SNew(SStaticMeshOptimizationPage);
	SkeletalMeshOptimizationPage = SNew(SSkeletalMeshOptimizationPage);
	ParticleSystemOptimizationPage = SNew(SParticleSystemOptimizationPage);
	BlueprintCompilePage = SNew(SBlueprintCompilePage);

	int32 CurrentlySelectedIndex = 0;
	const TArray<const PlatformInfo::FPlatformInfo*>& ConstAvailablePlatforms = FOptimizationAssistantHelpers::GetAvailablePlatforms();
	for (const PlatformInfo::FPlatformInfo* PlatformInfoItem : ConstAvailablePlatforms)
	{
		AvailablePlatforms.Add(MakeShared<FPlatformInfoHolder>(PlatformInfoItem));
	}
	FOptimizationAssistantHelpers::SetTargetPlatform(AvailablePlatforms[CurrentlySelectedIndex]->AvailablePlatform);
	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2.0f)
		[
			SNew(SBorder)
			.Padding(2)
			.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				.AutoWidth()
				[
					SNew(STextBlock)
					.Text(LOCTEXT("OptimizationRulesTitle", "配置优化规则"))
				]
			]
		]
		+SVerticalBox::Slot()
		.FillHeight(1.0f)
		.Padding(2.0f)
		[
			SNew(SScrollBox)
			.Visibility(EVisibility::Visible)

			+ SScrollBox::Slot()
			.Padding(0.0f, 0.0f, 8.0f, 0.0f)
			[
				SNew(SGridPanel)
				.FillColumn(1, 1.0f)

				+ SGridPanel::Slot(0, 0)
				.Padding(8.0f, 0.0f, 0.0f, 0.0f)
				.VAlign(VAlign_Top)
				[
					SNew(STextBlock)
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 13))
					.Text(LOCTEXT("GlobalSettingsSectionHeader", "GlobalSettings"))
				]

				+ SGridPanel::Slot(1, 0)
				.Padding(32.0f, 0.0f, 8.0f, 0.0f)
				[
					SNew(SGlobalSettingsPage)
				]

				+ SGridPanel::Slot(0, 1)
				.ColumnSpan(3)
				.Padding(0.0f, 16.0f)
				[
					SNew(SSeparator)
					.Orientation(Orient_Horizontal)
				]

				+ SGridPanel::Slot(0, 2)
				.Padding(8.0f, 0.0f, 0.0f, 0.0f)
				.VAlign(VAlign_Top)
				[
					SNew(STextBlock)
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 13))
					.Text(LOCTEXT("StaticMeshSectionHeader", "StaticMesh"))
				]

				+ SGridPanel::Slot(1, 2)
				.Padding(32.0f, 0.0f, 8.0f, 0.0f)
				[
					StaticMeshOptimizationPage.ToSharedRef()
				]

				+ SGridPanel::Slot(0, 3)
				.ColumnSpan(3)
				.Padding(0.0f, 16.0f)
				[
					SNew(SSeparator)
					.Orientation(Orient_Horizontal)
				]
		
				+ SGridPanel::Slot(0, 4)
				.Padding(8.0f, 0.0f, 0.0f, 0.0f)
				.VAlign(VAlign_Top)
				[
					SNew(STextBlock)
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 13))
					.Text(LOCTEXT("SkeletalMeshSectionHeader", "SkeletalMesh"))
				]

				+ SGridPanel::Slot(1, 4)
				.Padding(32.0f, 0.0f, 8.0f, 0.0f)
				[
					SkeletalMeshOptimizationPage.ToSharedRef()
				]

				+ SGridPanel::Slot(0, 5)
				.ColumnSpan(3)
				.Padding(0.0f, 16.0f)
				[
					SNew(SSeparator)
					.Orientation(Orient_Horizontal)
				]

				
				+ SGridPanel::Slot(0, 6)
				.Padding(8.0f, 0.0f, 0.0f, 0.0f)
				.VAlign(VAlign_Top)
				[
					SNew(STextBlock)
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 13))
					.Text(LOCTEXT("ParticleSystemSectionHeader", "ParticleSystem"))
				]

				+ SGridPanel::Slot(1, 6)
				.Padding(32.0f, 0.0f, 8.0f, 0.0f)
				[
					ParticleSystemOptimizationPage.ToSharedRef()
				]

				
				// Blueprint section
				+ SGridPanel::Slot(0, 7)
				.ColumnSpan(3)
				.Padding(0.0f, 16.0f)
				[
					SNew(SSeparator)
					.Orientation(Orient_Horizontal)
				]

				+ SGridPanel::Slot(0, 8)
				.Padding(8.0f, 0.0f, 0.0f, 0.0f)
				.VAlign(VAlign_Top)
				[
					SNew(STextBlock)
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 13))
					.Text(LOCTEXT("BlueprintSectionHeader", "Blueprint"))
				]

				+ SGridPanel::Slot(1, 8)
				.Padding(32.0f, 0.0f, 8.0f, 0.0f)
				[
					SNew(SBlueprintCompilePage)
				]
				
				/**
				// deploy section
				+ SGridPanel::Slot(0, 9)
				.ColumnSpan(3)
				.Padding(0.0f, 16.0f)
				[
					SNew(SSeparator)
					.Orientation(Orient_Horizontal)
				]

				+ SGridPanel::Slot(0, 10)
				.Padding(8.0f, 0.0f, 0.0f, 0.0f)
				.VAlign(VAlign_Top)
				[
					SNew(STextBlock)
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 13))
					.Text(LOCTEXT("DeploySectionHeader", "Deploy"))
				]

				+ SGridPanel::Slot(1, 10)
				.Padding(32.0f, 0.0f, 8.0f, 0.0f)
				[
					SNew(SProjectLauncherDeployPage, InModel)
				]

				// launch section
				+ SGridPanel::Slot(0, 11)
				.ColumnSpan(3)
				.Padding(0.0f, 16.0f)
				[
					SNew(SSeparator)
					.Orientation(Orient_Horizontal)
				]

				+ SGridPanel::Slot(0, 12)
				.Padding(8.0f, 0.0f, 0.0f, 0.0f)
				.VAlign(VAlign_Top)
				[
					SNew(STextBlock)
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 13))
					.Text(LOCTEXT("LaunchSectionHeader", "Launch"))
				]

				+ SGridPanel::Slot(1, 12)
				.HAlign(HAlign_Fill)
				.Padding(32.0f, 0.0f, 8.0f, 0.0f)
				[
					SNew(SProjectLauncherLaunchPage, InModel)
				]
				*/
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2.0f)
		[
			SNew(SBorder)
			.Padding(2)
			.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.Padding(FMargin(2.0f))
			    .AutoWidth()
				[
					SNew(SCheckBox)
					.IsChecked(EnableStaticMeshCheck)
					.OnCheckStateChanged_Lambda([=](ECheckBoxState NewState)
					{
						EnableStaticMeshCheck = NewState;
					})
					.Content()
					[
						SNew(STextBlock)
						.TextStyle(FEditorStyle::Get(), "ContentBrowser.TopBar.Font")
						.Text(FText::FromString(TEXT("StaticMesh")))
					]
				]

				+ SHorizontalBox::Slot()
				.Padding(FMargin(2.0f))
				.AutoWidth()
				[
					SNew(SCheckBox)
					.IsChecked(EnableSkeletalMeshCheck)
					.OnCheckStateChanged_Lambda([=](ECheckBoxState NewState)
					{
						EnableSkeletalMeshCheck = NewState;
					})
					.Content()
					[
						SNew(STextBlock)
						.TextStyle(FEditorStyle::Get(), "ContentBrowser.TopBar.Font")
						.Text(FText::FromString(TEXT("SkeletalMesh")))
					]
				]

				+ SHorizontalBox::Slot()
				.Padding(FMargin(2.0f))
				.AutoWidth()
				[
					SNew(SCheckBox)
					.IsChecked(EnableParticleSystemCheck)
					.OnCheckStateChanged_Lambda([=](ECheckBoxState NewState)
					{
						EnableParticleSystemCheck = NewState;
					})
					.Content()
					[
						SNew(STextBlock)
						.TextStyle(FEditorStyle::Get(), "ContentBrowser.TopBar.Font")
						.Text(FText::FromString(TEXT("ParticleSystem")))
					]
				]

				+ SHorizontalBox::Slot()
				.Padding(FMargin(2.0f))
				.AutoWidth()
				[
					SNew(SCheckBox)
					.IsChecked(EnableBlueprintCompileCheck)
					.OnCheckStateChanged_Lambda([=](ECheckBoxState NewState)
					{
						EnableBlueprintCompileCheck = NewState;
					})
					.Content()
					[
						SNew(STextBlock)
						.TextStyle(FEditorStyle::Get(), "ContentBrowser.TopBar.Font")
						.Text(FText::FromString(TEXT("CompileBlueprints")))
					]
				]

				+ SHorizontalBox::Slot()
				.Padding(FMargin(2.0f))
				.FillWidth(1.0f)
				[
					SNew(SBox)
				]
			]
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2.0f)
		[
			SNew(SBorder)
			.Padding(2)
			.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.Padding(FMargin(2.0f))
				.FillWidth(1.0f)
				[
					SNew(SSpacer)
				]
				+ SHorizontalBox::Slot()
				.Padding(FMargin(2.0f))
				.VAlign(VAlign_Center)
				.AutoWidth()
				[
					SNew(STextBlock)
					.TextStyle(FEditorStyle::Get(), "ContentBrowser.TopBar.Font")
					.Text(LOCTEXT("TargetPlatform", "TargetPlatform:"))
				]
				+ SHorizontalBox::Slot()
				.Padding(FMargin(2.0f))
				.AutoWidth()
				[
					SNew(SComboBox<TSharedPtr<FPlatformInfoHolder>>)
					.OptionsSource(&AvailablePlatforms)
					.InitiallySelectedItem(AvailablePlatforms[CurrentlySelectedIndex])
					.OnGenerateWidget(this, &SOptimizationAssistantView::GenerateTargetPlatformComboItem)
					.OnSelectionChanged(this, &SOptimizationAssistantView::HandleTargetPlatformComboChanged)
					[
						SNew(STextBlock).Text(this, &SOptimizationAssistantView::GetSelectedPlatformComboText)
					]
				]
				+ SHorizontalBox::Slot()
				.Padding(FMargin(2.0f))
				.AutoWidth()
				[
					SNew(SButton)
					.ButtonStyle(FEditorStyle::Get(), "FlatButton.Success")
					.OnClicked(this, &SOptimizationAssistantView::HandleCheckForWorld)
					.ToolTipText(LOCTEXT("CheckForWorldTips", "检查当前World加载的所有对象"))
					[
						SNew( SHorizontalBox )
						+ SHorizontalBox::Slot()
						.VAlign(VAlign_Center)
						.AutoWidth()
						[
							SNew(STextBlock)
							.TextStyle(FEditorStyle::Get(), "ContentBrowser.TopBar.Font")
							.Font(FEditorStyle::Get().GetFontStyle("FontAwesome.11"))
							.Text(FEditorFontGlyphs::Tasks)
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(4, 0, 0, 0)
						[
							SNew( STextBlock )
							.TextStyle( FEditorStyle::Get(), "ContentBrowser.TopBar.Font" )
							.Text(LOCTEXT("CheckForWorld", "CheckForWorld"))
						]
					]
				]

				+ SHorizontalBox::Slot()
				.Padding(FMargin(2.0f))
				.AutoWidth()
				[
					SNew(SButton)
					.ButtonStyle(FEditorStyle::Get(), "FlatButton.Success")
					.OnClicked(this, &SOptimizationAssistantView::HandleCheckForWorldAllAssets)
					.ToolTipText(LOCTEXT("CheckWorldAllAssetsTips", "检查当前World依赖的所有资源"))
					[
						SNew( SHorizontalBox )
						+ SHorizontalBox::Slot()
						.VAlign(VAlign_Center)
						.AutoWidth()
						[
							SNew(STextBlock)
							.TextStyle(FEditorStyle::Get(), "ContentBrowser.TopBar.Font")
							.Font(FEditorStyle::Get().GetFontStyle("FontAwesome.11"))
							.Text(FEditorFontGlyphs::Tasks)
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(4, 0, 0, 0)
						[
							SNew( STextBlock )
							.TextStyle( FEditorStyle::Get(), "ContentBrowser.TopBar.Font" )
							.Text(LOCTEXT("CheckWorldAllAssets", "CheckWorldAllAssets"))
						]
					]
				]

				+ SHorizontalBox::Slot()
				.Padding(FMargin(2.0f))
				.AutoWidth()
				[
					SNew(SButton)
					.ButtonStyle(FEditorStyle::Get(), "FlatButton.Success")
					.OnClicked(this, &SOptimizationAssistantView::HandleCheckForAllAssets)
					.ToolTipText(LOCTEXT("CheckForAllAssetsTips", "检查所有的资源"))
					[
						SNew( SHorizontalBox )
						+ SHorizontalBox::Slot()
						.VAlign(VAlign_Center)
						.AutoWidth()
						[
							SNew(STextBlock)
							.TextStyle(FEditorStyle::Get(), "ContentBrowser.TopBar.Font")
							.Font(FEditorStyle::Get().GetFontStyle("FontAwesome.11"))
							.Text(FEditorFontGlyphs::Tasks)
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(4, 0, 0, 0)
						[
							SNew( STextBlock )
							.TextStyle( FEditorStyle::Get(), "ContentBrowser.TopBar.Font" )
							.Text(LOCTEXT("CheckForAllAssets", "CheckForAllAssets"))
						]
					]
				]

				+ SHorizontalBox::Slot()
				.Padding(FMargin(2.0f))
				.AutoWidth()
				[
					SNew(SButton)
					.ButtonStyle(FEditorStyle::Get(), "FlatButton.Success")
					.OnClicked(this, &SOptimizationAssistantView::HandleSaveOptimizationRules)
					.ToolTipText(LOCTEXT("SaveOptimizationRules", "Save Optimization Rules."))
					[
						SNew( SHorizontalBox )
						+ SHorizontalBox::Slot()
						.VAlign(VAlign_Center)
						.AutoWidth()
						[
							SNew(STextBlock)
							.TextStyle(FEditorStyle::Get(), "ContentBrowser.TopBar.Font")
							.Font(FEditorStyle::Get().GetFontStyle("FontAwesome.11"))
							.Text(FEditorFontGlyphs::Floppy_O)
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Center)
						.Padding(4, 0, 0, 0)
						[
							SNew( STextBlock )
							.TextStyle( FEditorStyle::Get(), "ContentBrowser.TopBar.Font" )
							.Text(LOCTEXT("SaveOptimizationSettings", "SaveSettings"))
						]
					]
				]
			]
		]
	];
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION


FReply SOptimizationAssistantView::HandleCheckForWorld()
{
	GetMutableDefault<UGlobalCheckSettings>()->OptimizationCheckType = EOptimizationCheckType::OCT_World;
	ProcessOptimizationCheck();
	return FReply::Handled();
}

FReply SOptimizationAssistantView::HandleCheckForWorldAllAssets()
{
	GetMutableDefault<UGlobalCheckSettings>()->OptimizationCheckType = EOptimizationCheckType::OCT_WorldDependentAssets;
	ProcessOptimizationCheck();
	return FReply::Handled();
}

FReply SOptimizationAssistantView::HandleCheckForAllAssets()
{
	GetMutableDefault<UGlobalCheckSettings>()->OptimizationCheckType = EOptimizationCheckType::OCT_AllAssets;
	ProcessOptimizationCheck();
	return FReply::Handled();
}

FReply SOptimizationAssistantView::HandleSaveOptimizationRules()
{
	GetMutableDefault<UGlobalCheckSettings>()->UpdateDefaultConfigFile();
	GetMutableDefault<UStaticMeshOptimizationRules>()->UpdateDefaultConfigFile();
	GetMutableDefault<USkeletalMeshOptimizationRules>()->UpdateDefaultConfigFile();
	GetMutableDefault<UParticleSystemOptimizationRules>()->UpdateDefaultConfigFile();
	GetMutableDefault<UBlueprintCompileSettings>()->UpdateDefaultConfigFile();
	GConfig->Flush(false, FOptimizationAssistantModule::OptimizationAssistantIni);
	return FReply::Handled();
}

TSharedRef<SWidget> SOptimizationAssistantView::GenerateTargetPlatformComboItem(const TSharedPtr<FPlatformInfoHolder> InItem)
{
	return SNew(STextBlock).Text(InItem->AvailablePlatform->DisplayName);
}

void SOptimizationAssistantView::HandleTargetPlatformComboChanged(const TSharedPtr<FPlatformInfoHolder> Item, ESelectInfo::Type SelectInfo)
{
	FOptimizationAssistantHelpers::SetTargetPlatform(Item->AvailablePlatform);
}

FText SOptimizationAssistantView::GetSelectedPlatformComboText() const
{
	if (const PlatformInfo::FPlatformInfo* TargetPlatform = FOptimizationAssistantHelpers::GetTargetPlatform())
	{
		return TargetPlatform->DisplayName;
	}

	return FText::GetEmpty();
}

void SOptimizationAssistantView::ProcessOptimizationCheck()
{
	int32 TaskCount = 0;
	TaskCount = EnableStaticMeshCheck == ECheckBoxState::Checked ? ++TaskCount : TaskCount;
	TaskCount = EnableSkeletalMeshCheck == ECheckBoxState::Checked ? ++TaskCount : TaskCount;
	TaskCount = EnableParticleSystemCheck == ECheckBoxState::Checked ? ++TaskCount : TaskCount;
	TaskCount = EnableBlueprintCompileCheck == ECheckBoxState::Checked ? ++TaskCount : TaskCount;

	FScopedSlowTask SlowTask(TaskCount, FText::FromString(TEXT("Optimization Check")));
	SlowTask.MakeDialog(true);

	if (EnableStaticMeshCheck == ECheckBoxState::Checked)
	{
		SlowTask.EnterProgressFrame(1.0f);
		StaticMeshOptimizationPage->ProcessOptimizationCheck();
	}

	if (EnableSkeletalMeshCheck == ECheckBoxState::Checked)
	{
		SlowTask.EnterProgressFrame(1.0f);
		SkeletalMeshOptimizationPage->ProcessOptimizationCheck();
	}

	if (EnableParticleSystemCheck == ECheckBoxState::Checked)
	{
		SlowTask.EnterProgressFrame(1.0f);
		ParticleSystemOptimizationPage->ProcessOptimizationCheck();
	}

	if (EnableBlueprintCompileCheck == ECheckBoxState::Checked)
	{
		SlowTask.EnterProgressFrame(1.0f);
		BlueprintCompilePage->ProcessOptimizationCheck();
	}
}


#undef LOCTEXT_NAMESPACE


