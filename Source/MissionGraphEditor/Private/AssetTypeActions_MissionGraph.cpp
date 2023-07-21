#include "AssetTypeActions_MissionGraph.h"
#include "MissionGraphEditorPCH.h"
#include "MissionGraphAssetEditor/AssetEditor_MissionGraph.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions_MissionGraph"

FAssetTypeActions_MissionGraph::FAssetTypeActions_MissionGraph(EAssetTypeCategories::Type InAssetCategory)
	: MyAssetCategory(InAssetCategory)
{
}

FText FAssetTypeActions_MissionGraph::GetName() const
{
	return LOCTEXT("FMissionGraphAssetTypeActionsName", "Mission Graph");
}

FColor FAssetTypeActions_MissionGraph::GetTypeColor() const
{
	return FColor(129, 196, 115);
}

UClass* FAssetTypeActions_MissionGraph::GetSupportedClass() const
{
	return UMissionGraph::StaticClass();
}

void FAssetTypeActions_MissionGraph::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor)
{
	const EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;

	for (auto ObjIt = InObjects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		if (UMissionGraph* Graph = Cast<UMissionGraph>(*ObjIt))
		{
			TSharedRef<FAssetEditor_MissionGraph> NewGraphEditor(new FAssetEditor_MissionGraph());
			NewGraphEditor->InitMissionGraphAssetEditor(Mode, EditWithinLevelEditor, Graph);
		}
	}
}

uint32 FAssetTypeActions_MissionGraph::GetCategories()
{
	return MyAssetCategory;
}

//////////////////////////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
