#include "MissionGraphEditor.h"
#include "MissionGraphNodeFactory.h"
#include "AssetTypeActions_MissionGraph.h"
#include "MissionGraphAssetEditor/MissionGraphEditorStyle.h"

DEFINE_LOG_CATEGORY(MissionGraphEditor)

#define LOCTEXT_NAMESPACE "Editor_MissionGraph"

void FMissionGraphEditor::StartupModule()
{
	FMissionGraphEditorStyle::Initialize();

	GraphPanelNodeFactory_MissionGraph = MakeShareable(new FGraphPanelNodeFactory_MissionGraph());
	FEdGraphUtilities::RegisterVisualNodeFactory(GraphPanelNodeFactory_MissionGraph);

	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

	MissionGraphAssetCategoryBit = AssetTools.RegisterAdvancedAssetCategory(FName(TEXT("MissionGraph")), LOCTEXT("MissionGraphAssetCategory", "MissionGraph"));

	RegisterAssetTypeAction(AssetTools, MakeShareable(new FAssetTypeActions_MissionGraph(MissionGraphAssetCategoryBit)));
}


void FMissionGraphEditor::ShutdownModule()
{
	// Unregister all the asset types that we registered
	if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
	{
		IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
		for (int32 Index = 0; Index < CreatedAssetTypeActions.Num(); ++Index)
		{
			AssetTools.UnregisterAssetTypeActions(CreatedAssetTypeActions[Index].ToSharedRef());
		}
	}

	if (GraphPanelNodeFactory_MissionGraph.IsValid())
	{
		FEdGraphUtilities::UnregisterVisualNodeFactory(GraphPanelNodeFactory_MissionGraph);
		GraphPanelNodeFactory_MissionGraph.Reset();
	}

	FMissionGraphEditorStyle::Shutdown();
}

void FMissionGraphEditor::RegisterAssetTypeAction(IAssetTools& AssetTools, TSharedRef<IAssetTypeActions> Action)
{
	AssetTools.RegisterAssetTypeActions(Action);
	CreatedAssetTypeActions.Add(Action);
}

IMPLEMENT_MODULE(FMissionGraphEditor, MissionGraphEditor)

#undef LOCTEXT_NAMESPACE

