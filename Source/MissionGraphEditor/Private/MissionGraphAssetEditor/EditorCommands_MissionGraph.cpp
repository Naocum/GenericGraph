#include "MissionGraphAssetEditor/EditorCommands_MissionGraph.h"

#define LOCTEXT_NAMESPACE "EditorCommands_MissionGraph"

void FEditorCommands_MissionGraph::RegisterCommands()
{
	UI_COMMAND(GraphSettings, "Graph Settings", "Graph Settings", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(AutoArrange, "Auto Arrange", "Auto Arrange", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
