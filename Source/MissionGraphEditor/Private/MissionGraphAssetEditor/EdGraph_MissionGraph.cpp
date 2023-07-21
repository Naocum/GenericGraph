#include "MissionGraphAssetEditor/EdGraph_MissionGraph.h"
#include "MissionGraphEditorPCH.h"
#include "MissionGraph.h"
#include "MissionGraphAssetEditor/EdNode_MissionGraphNode.h"
#include "MissionGraphAssetEditor/EdNode_MissionGraphEdge.h"

UEdGraph_MissionGraph::UEdGraph_MissionGraph()
{

}

UEdGraph_MissionGraph::~UEdGraph_MissionGraph()
{

}

void UEdGraph_MissionGraph::RebuildMissionGraph()
{
	LOG_INFO(TEXT("UMissionGraphEdGraph::RebuildMissionGraph has been called"));

	UMissionGraph* Graph = GetMissionGraph();

	Clear();

	for (int i = 0; i < Nodes.Num(); ++i)
	{
		if (UEdNode_MissionGraphNode* EdNode = Cast<UEdNode_MissionGraphNode>(Nodes[i]))
		{
			if (EdNode->MissionGraphNode == nullptr)
				continue;

			UMissionGraphNode* MissionGraphNode = EdNode->MissionGraphNode;

			NodeMap.Add(MissionGraphNode, EdNode);

			Graph->AllNodes.Add(MissionGraphNode);

			for (int PinIdx = 0; PinIdx < EdNode->Pins.Num(); ++PinIdx)
			{
				UEdGraphPin* Pin = EdNode->Pins[PinIdx];

				if (Pin->Direction != EEdGraphPinDirection::EGPD_Output)
					continue;

				for (int LinkToIdx = 0; LinkToIdx < Pin->LinkedTo.Num(); ++LinkToIdx)
				{
					UMissionGraphNode* ChildNode = nullptr;
					if (UEdNode_MissionGraphNode* EdNode_Child = Cast<UEdNode_MissionGraphNode>(Pin->LinkedTo[LinkToIdx]->GetOwningNode()))
					{
						ChildNode = EdNode_Child->MissionGraphNode;
					}
					else if (UEdNode_MissionGraphEdge* EdNode_Edge = Cast<UEdNode_MissionGraphEdge>(Pin->LinkedTo[LinkToIdx]->GetOwningNode()))
					{
						UEdNode_MissionGraphNode* Child = EdNode_Edge->GetEndNode();;
						if (Child != nullptr)
						{
							ChildNode = Child->MissionGraphNode;
						}
					}

					if (ChildNode != nullptr)
					{
						MissionGraphNode->ChildrenNodes.Add(ChildNode);

						ChildNode->ParentNodes.Add(MissionGraphNode);
					}
					else
					{
						LOG_ERROR(TEXT("UEdGraph_MissionGraph::RebuildMissionGraph can't find child node"));
					}
				}
			}
		}
		else if (UEdNode_MissionGraphEdge* EdgeNode = Cast<UEdNode_MissionGraphEdge>(Nodes[i]))
		{
			UEdNode_MissionGraphNode* StartNode = EdgeNode->GetStartNode();
			UEdNode_MissionGraphNode* EndNode = EdgeNode->GetEndNode();
			UMissionGraphEdge* Edge = EdgeNode->MissionGraphEdge;

			if (StartNode == nullptr || EndNode == nullptr || Edge == nullptr)
			{
				LOG_ERROR(TEXT("UEdGraph_MissionGraph::RebuildMissionGraph add edge failed."));
				continue;
			}

			EdgeMap.Add(Edge, EdgeNode);

			Edge->Graph = Graph;
			Edge->Rename(nullptr, Graph, REN_DontCreateRedirectors | REN_DoNotDirty);
			Edge->StartNode = StartNode->MissionGraphNode;
			Edge->EndNode = EndNode->MissionGraphNode;
			Edge->StartNode->Edges.Add(Edge->EndNode, Edge);
		}
	}

	for (int i = 0; i < Graph->AllNodes.Num(); ++i)
	{
		UMissionGraphNode* Node = Graph->AllNodes[i];
		if (Node->ParentNodes.Num() == 0)
		{
			Graph->RootNodes.Add(Node);

			SortNodes(Node);
		}

		Node->Graph = Graph;
		Node->Rename(nullptr, Graph, REN_DontCreateRedirectors | REN_DoNotDirty);
	}

	Graph->RootNodes.Sort([&](const UMissionGraphNode& L, const UMissionGraphNode& R)
	{
		UEdNode_MissionGraphNode* EdNode_LNode = NodeMap[&L];
		UEdNode_MissionGraphNode* EdNode_RNode = NodeMap[&R];
		return EdNode_LNode->NodePosX < EdNode_RNode->NodePosX;
	});
}

UMissionGraph* UEdGraph_MissionGraph::GetMissionGraph() const
{
	return CastChecked<UMissionGraph>(GetOuter());
}

bool UEdGraph_MissionGraph::Modify(bool bAlwaysMarkDirty /*= true*/)
{
	bool Rtn = Super::Modify(bAlwaysMarkDirty);

	GetMissionGraph()->Modify();

	for (int32 i = 0; i < Nodes.Num(); ++i)
	{
		Nodes[i]->Modify();
	}

	return Rtn;
}

void UEdGraph_MissionGraph::Clear()
{
	UMissionGraph* Graph = GetMissionGraph();

	Graph->ClearGraph();
	NodeMap.Reset();
	EdgeMap.Reset();

	for (int i = 0; i < Nodes.Num(); ++i)
	{
		if (UEdNode_MissionGraphNode* EdNode = Cast<UEdNode_MissionGraphNode>(Nodes[i]))
		{
			UMissionGraphNode* MissionGraphNode = EdNode->MissionGraphNode;
			if (MissionGraphNode)
			{
				MissionGraphNode->ParentNodes.Reset();
				MissionGraphNode->ChildrenNodes.Reset();
				MissionGraphNode->Edges.Reset();
			}
		}
	}
}

void UEdGraph_MissionGraph::SortNodes(UMissionGraphNode* RootNode)
{
	int Level = 0;
	TArray<UMissionGraphNode*> CurrLevelNodes = { RootNode };
	TArray<UMissionGraphNode*> NextLevelNodes;
	TSet<UMissionGraphNode*> Visited;

	while (CurrLevelNodes.Num() != 0)
	{
		int32 LevelWidth = 0;
		for (int i = 0; i < CurrLevelNodes.Num(); ++i)
		{
			UMissionGraphNode* Node = CurrLevelNodes[i];
			Visited.Add(Node);

			auto Comp = [&](const UMissionGraphNode& L, const UMissionGraphNode& R)
			{
				UEdNode_MissionGraphNode* EdNode_LNode = NodeMap[&L];
				UEdNode_MissionGraphNode* EdNode_RNode = NodeMap[&R];
				return EdNode_LNode->NodePosX < EdNode_RNode->NodePosX;
			};

			Node->ChildrenNodes.Sort(Comp);
			Node->ParentNodes.Sort(Comp);

			for (int j = 0; j < Node->ChildrenNodes.Num(); ++j)
			{
				UMissionGraphNode* ChildNode = Node->ChildrenNodes[j];
				if(!Visited.Contains(ChildNode))
					NextLevelNodes.Add(Node->ChildrenNodes[j]);
			}
		}

		CurrLevelNodes = NextLevelNodes;
		NextLevelNodes.Reset();
		++Level;
	}
}

void UEdGraph_MissionGraph::PostEditUndo()
{
	Super::PostEditUndo();

	NotifyGraphChanged();
}

