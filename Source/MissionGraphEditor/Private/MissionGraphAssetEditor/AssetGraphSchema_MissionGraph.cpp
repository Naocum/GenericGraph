#include "MissionGraphAssetEditor/AssetGraphSchema_MissionGraph.h"
#include "ToolMenus.h"
#include "MissionGraphEditorPCH.h"
#include "MissionGraphAssetEditor/EdNode_MissionGraphNode.h"
#include "MissionGraphAssetEditor/EdNode_MissionGraphEdge.h"
#include "MissionGraphAssetEditor/ConnectionDrawingPolicy_MissionGraph.h"
#include "GraphEditorActions.h"
#include "Framework/Commands/MissionCommands.h"
#include "AutoLayout/ForceDirectedLayoutStrategy.h"
#include "AutoLayout/TreeLayoutStrategy.h"

#define LOCTEXT_NAMESPACE "AssetSchema_MissionGraph"

int32 UAssetGraphSchema_MissionGraph::CurrentCacheRefreshID = 0;


class FNodeVisitorCycleChecker
{
public:
	/** Check whether a loop in the graph would be caused by linking the passed-in nodes */
	bool CheckForLoop(UEdGraphNode* StartNode, UEdGraphNode* EndNode)
	{

		VisitedNodes.Add(StartNode);

		return TraverseNodes(EndNode);
	}

private:
	bool TraverseNodes(UEdGraphNode* Node)
	{
		VisitedNodes.Add(Node);

		for (auto MyPin : Node->Pins)
		{
			if (MyPin->Direction == EGPD_Output)
			{
				for (auto OtherPin : MyPin->LinkedTo)
				{
					UEdGraphNode* OtherNode = OtherPin->GetOwningNode();
					if (VisitedNodes.Contains(OtherNode))
					{
						// Only  an issue if this is a back-edge
						return false;
					}
					else if (!FinishedNodes.Contains(OtherNode))
					{
						// Only should traverse if this node hasn't been traversed
						if (!TraverseNodes(OtherNode))
							return false;
					}
				}
			}
		}

		VisitedNodes.Remove(Node);
		FinishedNodes.Add(Node);
		return true;
	};


	TSet<UEdGraphNode*> VisitedNodes;
	TSet<UEdGraphNode*> FinishedNodes;
};

UEdGraphNode* FAssetSchemaAction_MissionGraph_NewNode::PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode /*= true*/)
{
	UEdGraphNode* ResultNode = nullptr;

	if (NodeTemplate != nullptr)
	{
		const FScopedTransaction Transaction(LOCTEXT("MissionGraphEditorNewNode", "Mission Graph Editor: New Node"));
		ParentGraph->Modify();
		if (FromPin != nullptr)
			FromPin->Modify();

		NodeTemplate->Rename(nullptr, ParentGraph);
		ParentGraph->AddNode(NodeTemplate, true, bSelectNewNode);

		NodeTemplate->CreateNewGuid();
		NodeTemplate->PostPlacedNewNode();
		NodeTemplate->AllocateDefaultPins();
		NodeTemplate->AutowireNewNode(FromPin);

		NodeTemplate->NodePosX = Location.X;
		NodeTemplate->NodePosY = Location.Y;

		NodeTemplate->MissionGraphNode->SetFlags(RF_Transactional);
		NodeTemplate->SetFlags(RF_Transactional);

		ResultNode = NodeTemplate;
	}

	return ResultNode;
}

void FAssetSchemaAction_MissionGraph_NewNode::AddReferencedObjects(FReferenceCollector& Collector)
{
	FEdGraphSchemaAction::AddReferencedObjects(Collector);
	Collector.AddReferencedObject(NodeTemplate);
}

UEdGraphNode* FAssetSchemaAction_MissionGraph_NewEdge::PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode /*= true*/)
{
	UEdGraphNode* ResultNode = nullptr;

	if (NodeTemplate != nullptr)
	{
		const FScopedTransaction Transaction(LOCTEXT("MissionGraphEditorNewEdge", "Mission Graph Editor: New Edge"));
		ParentGraph->Modify();
		if (FromPin != nullptr)
			FromPin->Modify();

		NodeTemplate->Rename(nullptr, ParentGraph);
		ParentGraph->AddNode(NodeTemplate, true, bSelectNewNode);

		NodeTemplate->CreateNewGuid();
		NodeTemplate->PostPlacedNewNode();
		NodeTemplate->AllocateDefaultPins();
		NodeTemplate->AutowireNewNode(FromPin);

		NodeTemplate->NodePosX = Location.X;
		NodeTemplate->NodePosY = Location.Y;

		NodeTemplate->MissionGraphEdge->SetFlags(RF_Transactional);
		NodeTemplate->SetFlags(RF_Transactional);

		ResultNode = NodeTemplate;
	}
	
	return ResultNode;
}

void FAssetSchemaAction_MissionGraph_NewEdge::AddReferencedObjects(FReferenceCollector& Collector)
{
	FEdGraphSchemaAction::AddReferencedObjects(Collector);
	Collector.AddReferencedObject(NodeTemplate);
}

void UAssetGraphSchema_MissionGraph::GetBreakLinkToSubMenuActions(UToolMenu* Menu, UEdGraphPin* InGraphPin)
{
	// Make sure we have a unique name for every entry in the list
	TMap< FString, uint32 > LinkTitleCount;

	FToolMenuSection& Section = Menu->FindOrAddSection("MissionGraphAssetGraphSchemaPinActions");

	// Add all the links we could break from
	for (TArray<class UEdGraphPin*>::TConstIterator Links(InGraphPin->LinkedTo); Links; ++Links)
	{
		UEdGraphPin* Pin = *Links;
		FString TitleString = Pin->GetOwningNode()->GetNodeTitle(ENodeTitleType::ListView).ToString();
		FText Title = FText::FromString(TitleString);
		if (Pin->PinName != TEXT(""))
		{
			TitleString = FString::Printf(TEXT("%s (%s)"), *TitleString, *Pin->PinName.ToString());

			// Add name of connection if possible
			FFormatNamedArguments Args;
			Args.Add(TEXT("NodeTitle"), Title);
			Args.Add(TEXT("PinName"), Pin->GetDisplayName());
			Title = FText::Format(LOCTEXT("BreakDescPin", "{NodeTitle} ({PinName})"), Args);
		}

		uint32& Count = LinkTitleCount.FindOrAdd(TitleString);

		FText Description;
		FFormatNamedArguments Args;
		Args.Add(TEXT("NodeTitle"), Title);
		Args.Add(TEXT("NumberOfNodes"), Count);

		if (Count == 0)
		{
			Description = FText::Format(LOCTEXT("BreakDesc", "Break link to {NodeTitle}"), Args);
		}
		else
		{
			Description = FText::Format(LOCTEXT("BreakDescMulti", "Break link to {NodeTitle} ({NumberOfNodes})"), Args);
		}
		++Count;

		Section.AddMenuEntry(NAME_None, Description, Description, FSlateIcon(), FUIAction(
			FExecuteAction::CreateUObject(this, &UAssetGraphSchema_MissionGraph::BreakSinglePinLink, const_cast<UEdGraphPin*>(InGraphPin), *Links)));
	}
}

EGraphType UAssetGraphSchema_MissionGraph::GetGraphType(const UEdGraph* TestEdGraph) const
{
	return GT_StateMachine;
}

void UAssetGraphSchema_MissionGraph::GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
	UMissionGraph* Graph = CastChecked<UMissionGraph>(ContextMenuBuilder.CurrentGraph->GetOuter());

	if (Graph->NodeType == nullptr)
	{
		return;
	}

	const bool bNoParent = (ContextMenuBuilder.FromPin == NULL);

	const FText AddToolTip = LOCTEXT("NewMissionGraphNodeTooltip", "Add node here");

	TSet<TSubclassOf<UMissionGraphNode> > Visited;

	FText Desc = Graph->NodeType.GetDefaultObject()->ContextMenuName;

	if (Desc.IsEmpty())
	{
		FString Title = Graph->NodeType->GetName();
		Title.RemoveFromEnd("_C");
		Desc = FText::FromString(Title);
	}

	if (!Graph->NodeType->HasAnyClassFlags(CLASS_Abstract))
	{
		TSharedPtr<FAssetSchemaAction_MissionGraph_NewNode> NewNodeAction(new FAssetSchemaAction_MissionGraph_NewNode(LOCTEXT("MissionGraphNodeAction", "Mission Graph Node"), Desc, AddToolTip, 0));
		NewNodeAction->NodeTemplate = NewObject<UEdNode_MissionGraphNode>(ContextMenuBuilder.OwnerOfTemporaries);
		NewNodeAction->NodeTemplate->MissionGraphNode = NewObject<UMissionGraphNode>(NewNodeAction->NodeTemplate, Graph->NodeType);
		NewNodeAction->NodeTemplate->MissionGraphNode->Graph = Graph;
		ContextMenuBuilder.AddAction(NewNodeAction);

		Visited.Add(Graph->NodeType);
	}

	for (TObjectIterator<UClass> It; It; ++It)
	{
		if (It->IsChildOf(Graph->NodeType) && !It->HasAnyClassFlags(CLASS_Abstract) && !Visited.Contains(*It))
		{
			TSubclassOf<UMissionGraphNode> NodeType = *It;

			if (It->GetName().StartsWith("REINST") || It->GetName().StartsWith("SKEL"))
				continue;

			if (!Graph->GetClass()->IsChildOf(NodeType.GetDefaultObject()->CompatibleGraphType))
				continue;

			Desc = NodeType.GetDefaultObject()->ContextMenuName;

			if (Desc.IsEmpty())
			{
				FString Title = NodeType->GetName();
				Title.RemoveFromEnd("_C");
				Desc = FText::FromString(Title);
			}

			TSharedPtr<FAssetSchemaAction_MissionGraph_NewNode> Action(new FAssetSchemaAction_MissionGraph_NewNode(LOCTEXT("MissionGraphNodeAction", "Mission Graph Node"), Desc, AddToolTip, 0));
			Action->NodeTemplate = NewObject<UEdNode_MissionGraphNode>(ContextMenuBuilder.OwnerOfTemporaries);
			Action->NodeTemplate->MissionGraphNode = NewObject<UMissionGraphNode>(Action->NodeTemplate, NodeType);
			Action->NodeTemplate->MissionGraphNode->Graph = Graph;
			ContextMenuBuilder.AddAction(Action);

			Visited.Add(NodeType);
		}
	}
}

void UAssetGraphSchema_MissionGraph::GetContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const
{
	if (Context->Pin)
	{
		{
			FToolMenuSection& Section = Menu->AddSection("MissionGraphAssetGraphSchemaNodeActions", LOCTEXT("PinActionsMenuHeader", "Pin Actions"));
			// Only display the 'Break Links' option if there is a link to break!
			if (Context->Pin->LinkedTo.Num() > 0)
			{
				Section.AddMenuEntry(FGraphEditorCommands::Get().BreakPinLinks);

				// add sub menu for break link to
				if (Context->Pin->LinkedTo.Num() > 1)
				{
					Section.AddSubMenu(
						"BreakLinkTo",
						LOCTEXT("BreakLinkTo", "Break Link To..."),
						LOCTEXT("BreakSpecificLinks", "Break a specific link..."),
						FNewToolMenuDelegate::CreateUObject((UAssetGraphSchema_MissionGraph* const)this, &UAssetGraphSchema_MissionGraph::GetBreakLinkToSubMenuActions, const_cast<UEdGraphPin*>(Context->Pin)));
				}
				else
				{
					((UAssetGraphSchema_MissionGraph* const)this)->GetBreakLinkToSubMenuActions(Menu, const_cast<UEdGraphPin*>(Context->Pin));
				}
			}
		}
	}
	else if (Context->Node)
	{
		{
			FToolMenuSection& Section = Menu->AddSection("MissionGraphAssetGraphSchemaNodeActions", LOCTEXT("ClassActionsMenuHeader", "Node Actions"));
			Section.AddMenuEntry(FMissionCommands::Get().Delete);
			Section.AddMenuEntry(FMissionCommands::Get().Cut);
			Section.AddMenuEntry(FMissionCommands::Get().Copy);
			Section.AddMenuEntry(FMissionCommands::Get().Duplicate);

			Section.AddMenuEntry(FGraphEditorCommands::Get().BreakNodeLinks);
		}
	}

	Super::GetContextMenuActions(Menu, Context);
}

const FPinConnectionResponse UAssetGraphSchema_MissionGraph::CanCreateConnection(const UEdGraphPin* A, const UEdGraphPin* B) const
{
	// Make sure the pins are not on the same node
	if (A->GetOwningNode() == B->GetOwningNode())
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("PinErrorSameNode", "Can't connect node to itself"));
	}

	const UEdGraphPin *Out = A;
	const UEdGraphPin *In = B;

	UEdNode_MissionGraphNode* EdNode_Out = Cast<UEdNode_MissionGraphNode>(Out->GetOwningNode());
	UEdNode_MissionGraphNode* EdNode_In = Cast<UEdNode_MissionGraphNode>(In->GetOwningNode());

	if (EdNode_Out == nullptr || EdNode_In == nullptr)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("PinError", "Not a valid UMissionGraphEdNode"));
	}
		
	//Determine if we can have cycles or not
	bool bAllowCycles = false;
	auto EdGraph = Cast<UEdGraph_MissionGraph>(Out->GetOwningNode()->GetGraph());
	if (EdGraph != nullptr)
	{
		bAllowCycles = EdGraph->GetMissionGraph()->bCanBeCyclical;
	}

	// check for cycles
	FNodeVisitorCycleChecker CycleChecker;
	if (!bAllowCycles && !CycleChecker.CheckForLoop(Out->GetOwningNode(), In->GetOwningNode()))
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("PinErrorCycle", "Can't create a graph cycle"));
	}

	FText ErrorMessage;
	if (!EdNode_Out->MissionGraphNode->CanCreateConnectionTo(EdNode_In->MissionGraphNode, EdNode_Out->GetOutputPin()->LinkedTo.Num(), ErrorMessage))
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, ErrorMessage);
	}
	if (!EdNode_In->MissionGraphNode->CanCreateConnectionFrom(EdNode_Out->MissionGraphNode, EdNode_In->GetInputPin()->LinkedTo.Num(), ErrorMessage))
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, ErrorMessage);
	}


	if (EdNode_Out->MissionGraphNode->GetGraph()->bEdgeEnabled)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_MAKE_WITH_CONVERSION_NODE, LOCTEXT("PinConnect", "Connect nodes with edge"));
	}
	else
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_MAKE, LOCTEXT("PinConnect", "Connect nodes"));
	}
}

bool UAssetGraphSchema_MissionGraph::TryCreateConnection(UEdGraphPin* A, UEdGraphPin* B) const
{
	// We don't actually care about the pin, we want the node that is being dragged between
	UEdNode_MissionGraphNode* NodeA = Cast<UEdNode_MissionGraphNode>(A->GetOwningNode());
	UEdNode_MissionGraphNode* NodeB = Cast<UEdNode_MissionGraphNode>(B->GetOwningNode());

	// Check that this edge doesn't already exist
	for (UEdGraphPin *TestPin : NodeA->GetOutputPin()->LinkedTo)
	{
		UEdGraphNode* ChildNode = TestPin->GetOwningNode();
		if (UEdNode_MissionGraphEdge* EdNode_Edge = Cast<UEdNode_MissionGraphEdge>(ChildNode))
		{
			ChildNode = EdNode_Edge->GetEndNode();
		}

		if (ChildNode == NodeB)
			return false;
	}

	if (NodeA && NodeB)
	{
		// Always create connections from node A to B, don't allow adding in reverse
		Super::TryCreateConnection(NodeA->GetOutputPin(), NodeB->GetInputPin());
		return true;
	}
	else
	{
		return false;
	}
}

bool UAssetGraphSchema_MissionGraph::CreateAutomaticConversionNodeAndConnections(UEdGraphPin* A, UEdGraphPin* B) const
{
	UEdNode_MissionGraphNode* NodeA = Cast<UEdNode_MissionGraphNode>(A->GetOwningNode());
	UEdNode_MissionGraphNode* NodeB = Cast<UEdNode_MissionGraphNode>(B->GetOwningNode());

	// Are nodes and pins all valid?
	if (!NodeA || !NodeA->GetOutputPin() || !NodeB || !NodeB->GetInputPin())
		return false;
	
	UMissionGraph* Graph = NodeA->MissionGraphNode->GetGraph();

	FVector2D InitPos((NodeA->NodePosX + NodeB->NodePosX) / 2, (NodeA->NodePosY + NodeB->NodePosY) / 2);

	FAssetSchemaAction_MissionGraph_NewEdge Action;
	Action.NodeTemplate = NewObject<UEdNode_MissionGraphEdge>(NodeA->GetGraph());
	Action.NodeTemplate->SetEdge(NewObject<UMissionGraphEdge>(Action.NodeTemplate, Graph->EdgeType));
	UEdNode_MissionGraphEdge* EdgeNode = Cast<UEdNode_MissionGraphEdge>(Action.PerformAction(NodeA->GetGraph(), nullptr, InitPos, false));

	// Always create connections from node A to B, don't allow adding in reverse
	EdgeNode->CreateConnections(NodeA, NodeB);

	return true;
}

class FConnectionDrawingPolicy* UAssetGraphSchema_MissionGraph::CreateConnectionDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID, float InZoomFactor, const FSlateRect& InClippingRect, class FSlateWindowElementList& InDrawElements, class UEdGraph* InGraphObj) const
{
	return new FConnectionDrawingPolicy_MissionGraph(InBackLayerID, InFrontLayerID, InZoomFactor, InClippingRect, InDrawElements, InGraphObj);
}

FLinearColor UAssetGraphSchema_MissionGraph::GetPinTypeColor(const FEdGraphPinType& PinType) const
{
	return FColor::White;
}

void UAssetGraphSchema_MissionGraph::BreakNodeLinks(UEdGraphNode& TargetNode) const
{
	const FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "GraphEd_BreakNodeLinks", "Break Node Links"));

	Super::BreakNodeLinks(TargetNode);
}

void UAssetGraphSchema_MissionGraph::BreakPinLinks(UEdGraphPin& TargetPin, bool bSendsNodeNotifcation) const
{
	const FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "GraphEd_BreakPinLinks", "Break Pin Links"));

	Super::BreakPinLinks(TargetPin, bSendsNodeNotifcation);
}

void UAssetGraphSchema_MissionGraph::BreakSinglePinLink(UEdGraphPin* SourcePin, UEdGraphPin* TargetPin) const
{
	const FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "GraphEd_BreakSinglePinLink", "Break Pin Link"));

	Super::BreakSinglePinLink(SourcePin, TargetPin);
}

UEdGraphPin* UAssetGraphSchema_MissionGraph::DropPinOnNode(UEdGraphNode* InTargetNode, const FName& InSourcePinName, const FEdGraphPinType& InSourcePinType, EEdGraphPinDirection InSourcePinDirection) const
{
	UEdNode_MissionGraphNode* EdNode = Cast<UEdNode_MissionGraphNode>(InTargetNode);
	switch (InSourcePinDirection)
	{
	case EGPD_Input:
		return EdNode->GetOutputPin();
	case EGPD_Output:
		return EdNode->GetInputPin();
	default:
		return nullptr;
	}
}

bool UAssetGraphSchema_MissionGraph::SupportsDropPinOnNode(UEdGraphNode* InTargetNode, const FEdGraphPinType& InSourcePinType, EEdGraphPinDirection InSourcePinDirection, FText& OutErrorMessage) const
{
	return Cast<UEdNode_MissionGraphNode>(InTargetNode) != nullptr;
}

bool UAssetGraphSchema_MissionGraph::IsCacheVisualizationOutOfDate(int32 InVisualizationCacheID) const
{
	return CurrentCacheRefreshID != InVisualizationCacheID;
}

int32 UAssetGraphSchema_MissionGraph::GetCurrentVisualizationCacheID() const
{
	return CurrentCacheRefreshID;
}

void UAssetGraphSchema_MissionGraph::ForceVisualizationCacheClear() const
{
	++CurrentCacheRefreshID;
}

#undef LOCTEXT_NAMESPACE
