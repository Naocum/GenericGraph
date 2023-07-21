#include "MissionGraphAssetEditor/EdNode_MissionGraphNode.h"
#include "MissionGraphAssetEditor/EdGraph_MissionGraph.h"
#include "Kismet2/Kismet2NameValidators.h"
#include "Kismet2/BlueprintEditorUtils.h"

#define LOCTEXT_NAMESPACE "EdNode_MissionGraph"

UEdNode_MissionGraphNode::UEdNode_MissionGraphNode()
{
	bCanRenameNode = true;
}

UEdNode_MissionGraphNode::~UEdNode_MissionGraphNode()
{

}

void UEdNode_MissionGraphNode::AllocateDefaultPins()
{
	CreatePin(EGPD_Input, "MultipleNodes", FName(), TEXT("In"));
	CreatePin(EGPD_Output, "MultipleNodes", FName(), TEXT("Out"));
}

UEdGraph_MissionGraph* UEdNode_MissionGraphNode::GetMissionGraphEdGraph()
{
	return Cast<UEdGraph_MissionGraph>(GetGraph());
}

FText UEdNode_MissionGraphNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (MissionGraphNode == nullptr)
	{
		return Super::GetNodeTitle(TitleType);
	}
	else
	{
		return MissionGraphNode->GetNodeTitle();
	}
}

void UEdNode_MissionGraphNode::PrepareForCopying()
{
	MissionGraphNode->Rename(nullptr, this, REN_DontCreateRedirectors | REN_DoNotDirty);
}

void UEdNode_MissionGraphNode::AutowireNewNode(UEdGraphPin* FromPin)
{
	Super::AutowireNewNode(FromPin);

	if (FromPin != nullptr)
	{
		if (GetSchema()->TryCreateConnection(FromPin, GetInputPin()))
		{
			FromPin->GetOwningNode()->NodeConnectionListChanged();
		}
	}
}

void UEdNode_MissionGraphNode::SetMissionGraphNode(UMissionGraphNode* InNode)
{
	MissionGraphNode = InNode;
}

FLinearColor UEdNode_MissionGraphNode::GetBackgroundColor() const
{
	return MissionGraphNode == nullptr ? FLinearColor::Black : MissionGraphNode->GetBackgroundColor();
}

UEdGraphPin* UEdNode_MissionGraphNode::GetInputPin() const
{
	return Pins[0];
}

UEdGraphPin* UEdNode_MissionGraphNode::GetOutputPin() const
{
	return Pins[1];
}

void UEdNode_MissionGraphNode::PostEditUndo()
{
	UEdGraphNode::PostEditUndo();
}

#undef LOCTEXT_NAMESPACE
