#include "MissionGraphAssetEditor/EdNode_MissionGraphEdge.h"
#include "MissionGraphEdge.h"
#include "MissionGraphAssetEditor/EdNode_MissionGraphNode.h"

#define LOCTEXT_NAMESPACE "EdNode_MissionGraphEdge"

UEdNode_MissionGraphEdge::UEdNode_MissionGraphEdge()
{
	bCanRenameNode = true;
}

void UEdNode_MissionGraphEdge::SetEdge(UMissionGraphEdge* Edge)
{
	MissionGraphEdge = Edge;
}

void UEdNode_MissionGraphEdge::AllocateDefaultPins()
{
	UEdGraphPin* Inputs = CreatePin(EGPD_Input, TEXT("Edge"), FName(), TEXT("In"));
	Inputs->bHidden = true;
	UEdGraphPin* Outputs = CreatePin(EGPD_Output, TEXT("Edge"), FName(), TEXT("Out"));
	Outputs->bHidden = true;
}

FText UEdNode_MissionGraphEdge::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (MissionGraphEdge)
	{
		return MissionGraphEdge->GetNodeTitle();
	}
	return FText();
}

void UEdNode_MissionGraphEdge::PinConnectionListChanged(UEdGraphPin* Pin)
{
	if (Pin->LinkedTo.Num() == 0)
	{
		// Commit suicide; transitions must always have an input and output connection
		Modify();

		// Our parent graph will have our graph in SubGraphs so needs to be modified to record that.
		if (UEdGraph* ParentGraph = GetGraph())
		{
			ParentGraph->Modify();
		}

		DestroyNode();
	}
}

void UEdNode_MissionGraphEdge::PrepareForCopying()
{
	MissionGraphEdge->Rename(nullptr, this, REN_DontCreateRedirectors | REN_DoNotDirty);
}

void UEdNode_MissionGraphEdge::CreateConnections(UEdNode_MissionGraphNode* Start, UEdNode_MissionGraphNode* End)
{
	Pins[0]->Modify();
	Pins[0]->LinkedTo.Empty();

	Start->GetOutputPin()->Modify();
	Pins[0]->MakeLinkTo(Start->GetOutputPin());

	// This to next
	Pins[1]->Modify();
	Pins[1]->LinkedTo.Empty();

	End->GetInputPin()->Modify();
	Pins[1]->MakeLinkTo(End->GetInputPin());
}

UEdNode_MissionGraphNode* UEdNode_MissionGraphEdge::GetStartNode()
{
	if (Pins[0]->LinkedTo.Num() > 0)
	{
		return Cast<UEdNode_MissionGraphNode>(Pins[0]->LinkedTo[0]->GetOwningNode());
	}
	else
	{
		return nullptr;
	}
}

UEdNode_MissionGraphNode* UEdNode_MissionGraphEdge::GetEndNode()
{
	if (Pins[1]->LinkedTo.Num() > 0)
	{
		return Cast<UEdNode_MissionGraphNode>(Pins[1]->LinkedTo[0]->GetOwningNode());
	}
	else
	{
		return nullptr;
	}
}

#undef LOCTEXT_NAMESPACE

