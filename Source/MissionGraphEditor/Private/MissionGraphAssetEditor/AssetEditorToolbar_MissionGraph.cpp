#include "MissionGraphAssetEditor/AssetEditorToolbar_MissionGraph.h"
#include "MissionGraphAssetEditor/AssetEditor_MissionGraph.h"
#include "MissionGraphAssetEditor/EditorCommands_MissionGraph.h"
#include "MissionGraphAssetEditor/MissionGraphEditorStyle.h"

#define LOCTEXT_NAMESPACE "AssetEditorToolbar_MissionGraph"

void FAssetEditorToolbar_MissionGraph::AddMissionGraphToolbar(TSharedPtr<FExtender> Extender)
{
	check(MissionGraphEditor.IsValid());
	TSharedPtr<FAssetEditor_MissionGraph> MissionGraphEditorPtr = MissionGraphEditor.Pin();

	TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);
	ToolbarExtender->AddToolBarExtension("Asset", EExtensionHook::After, MissionGraphEditorPtr->GetToolkitCommands(), FToolBarExtensionDelegate::CreateSP( this, &FAssetEditorToolbar_MissionGraph::FillMissionGraphToolbar ));
	MissionGraphEditorPtr->AddToolbarExtender(ToolbarExtender);
}

void FAssetEditorToolbar_MissionGraph::FillMissionGraphToolbar(FToolBarBuilder& ToolbarBuilder)
{
	check(MissionGraphEditor.IsValid());
	TSharedPtr<FAssetEditor_MissionGraph> MissionGraphEditorPtr = MissionGraphEditor.Pin();

	ToolbarBuilder.BeginSection("Mission Graph");
	{
		ToolbarBuilder.AddToolBarButton(FEditorCommands_MissionGraph::Get().GraphSettings,
			NAME_None,
			LOCTEXT("GraphSettings_Label", "Graph Settings"),
			LOCTEXT("GraphSettings_ToolTip", "Show the Graph Settings"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.GameSettings"));
	}
	ToolbarBuilder.EndSection();

	ToolbarBuilder.BeginSection("Util");
	{
		ToolbarBuilder.AddToolBarButton(FEditorCommands_MissionGraph::Get().AutoArrange,
			NAME_None,
			LOCTEXT("AutoArrange_Label", "Auto Arrange"),
			LOCTEXT("AutoArrange_ToolTip", "Auto arrange nodes, not perfect, but still handy"),
			FSlateIcon(FMissionGraphEditorStyle::GetStyleSetName(), "MissionGraphEditor.AutoArrange"));
	}
	ToolbarBuilder.EndSection();

}


#undef LOCTEXT_NAMESPACE
