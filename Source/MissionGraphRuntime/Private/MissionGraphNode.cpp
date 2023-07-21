#include "MissionGraphNode.h"
#include "MissionGraph.h"

#define LOCTEXT_NAMESPACE "MissionGraphNode"

UMissionGraphNode::UMissionGraphNode()
{
#if WITH_EDITORONLY_DATA
	CompatibleGraphType = UMissionGraph::StaticClass();

	BackgroundColor = FLinearColor::Black;
#endif
}

UMissionGraphNode::~UMissionGraphNode()
{
}

UMissionGraphEdge* UMissionGraphNode::GetEdge(UMissionGraphNode* ChildNode)
{
	return Edges.Contains(ChildNode) ? Edges.FindChecked(ChildNode) : nullptr;
}

FText UMissionGraphNode::GetDescription_Implementation() const
{
	return LOCTEXT("NodeDesc", "Mission Graph Node");
}

#if WITH_EDITOR
bool UMissionGraphNode::IsNameEditable() const
{
	return true;
}

FLinearColor UMissionGraphNode::GetBackgroundColor() const
{
	return BackgroundColor;
}

FText UMissionGraphNode::GetNodeTitle() const
{
	return NodeTitle.IsEmpty() ? GetDescription() : NodeTitle;
}

void UMissionGraphNode::SetNodeTitle(const FText& NewTitle)
{
	NodeTitle = NewTitle;
}

bool UMissionGraphNode::CanCreateConnection(UMissionGraphNode* Other, FText& ErrorMessage)
{	
	return true;
}

bool UMissionGraphNode::CanCreateConnectionTo(UMissionGraphNode* Other, int32 NumberOfChildrenNodes, FText& ErrorMessage)
{
	if (ChildrenLimitType == ENodeLimit::Limited && NumberOfChildrenNodes >= ChildrenLimit)
	{
		ErrorMessage = FText::FromString("Children limit exceeded");
		return false;
	}

	return CanCreateConnection(Other, ErrorMessage);
}

bool UMissionGraphNode::CanCreateConnectionFrom(UMissionGraphNode* Other, int32 NumberOfParentNodes, FText& ErrorMessage)
{
	if (ParentLimitType == ENodeLimit::Limited && NumberOfParentNodes >= ParentLimit)
	{
		ErrorMessage = FText::FromString("Parent limit exceeded");
		return false;
	}

	return true;
}


#endif

bool UMissionGraphNode::IsLeafNode() const
{
	return ChildrenNodes.Num() == 0;
}

UMissionGraph* UMissionGraphNode::GetGraph() const
{
	return Graph;
}

#undef LOCTEXT_NAMESPACE
