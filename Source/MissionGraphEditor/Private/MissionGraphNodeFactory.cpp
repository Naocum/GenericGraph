#include "MissionGraphNodeFactory.h"
#include <EdGraph/EdGraphNode.h>
#include "MissionGraphAssetEditor/SEdNode_MissionGraphEdge.h"
#include "MissionGraphAssetEditor/EdNode_MissionGraphNode.h"
#include "MissionGraphAssetEditor/SEdNode_MissionGraphNode.h"
#include "MissionGraphAssetEditor/EdNode_MissionGraphEdge.h"

TSharedPtr<class SGraphNode> FGraphPanelNodeFactory_MissionGraph::CreateNode(UEdGraphNode* Node) const
{
	if (UEdNode_MissionGraphNode* EdNode_GraphNode = Cast<UEdNode_MissionGraphNode>(Node))
	{
		return SNew(SEdNode_MissionGraphNode, EdNode_GraphNode);
	}
	else if (UEdNode_MissionGraphEdge* EdNode_Edge = Cast<UEdNode_MissionGraphEdge>(Node))
	{
		return SNew(SEdNode_MissionGraphEdge, EdNode_Edge);
	}
	return nullptr;
}

