#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"
#include "MissionGraphNode.generated.h"

class UMissionGraph;
class UMissionGraphEdge;

UENUM(BlueprintType)
enum class ENodeLimit : uint8
{
	Unlimited,
    Limited
};


UCLASS(Blueprintable)
class GENERICGRAPHRUNTIME_API UMissionGraphNode : public UObject
{
	GENERATED_BODY()

public:
	UMissionGraphNode();
	virtual ~UMissionGraphNode();

	UPROPERTY(VisibleDefaultsOnly, Category = "MissionGraphNode")
	UMissionGraph* Graph;

	UPROPERTY(BlueprintReadOnly, Category = "MissionGraphNode")
	TArray<UMissionGraphNode*> ParentNodes;

	UPROPERTY(BlueprintReadOnly, Category = "MissionGraphNode")
	TArray<UMissionGraphNode*> ChildrenNodes;

	UPROPERTY(BlueprintReadOnly, Category = "MissionGraphNode")
	TMap<UMissionGraphNode*, UMissionGraphEdge*> Edges;

	UFUNCTION(BlueprintCallable, Category = "MissionGraphNode")
	virtual UMissionGraphEdge* GetEdge(UMissionGraphNode* ChildNode);

	UFUNCTION(BlueprintCallable, Category = "MissionGraphNode")
	bool IsLeafNode() const;

	UFUNCTION(BlueprintCallable, Category = "MissionGraphNode")
	UMissionGraph* GetGraph() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "MissionGraphNode")
	FText GetDescription() const;
	virtual FText GetDescription_Implementation() const;

	//////////////////////////////////////////////////////////////////////////
#if WITH_EDITORONLY_DATA
	UPROPERTY(EditDefaultsOnly, Category = "MissionGraphNode_Editor")
	FText NodeTitle;

	UPROPERTY(VisibleDefaultsOnly, Category = "MissionGraphNode_Editor")
	TSubclassOf<UMissionGraph> CompatibleGraphType;

	UPROPERTY(EditDefaultsOnly, Category = "MissionGraphNode_Editor")
	FLinearColor BackgroundColor;

	UPROPERTY(EditDefaultsOnly, Category = "MissionGraphNode_Editor")
	FText ContextMenuName;

	UPROPERTY(EditDefaultsOnly, Category = "MissionGraphNode_Editor")
	ENodeLimit ParentLimitType;

	UPROPERTY(EditDefaultsOnly, Category = "MissionGraphNode_Editor" ,meta = (ClampMin = "0",EditCondition = "ParentLimitType == ENodeLimit::Limited", EditConditionHides))
	int32 ParentLimit;

	UPROPERTY(EditDefaultsOnly, Category = "MissionGraphNode_Editor")
	ENodeLimit ChildrenLimitType;

	UPROPERTY(EditDefaultsOnly, Category = "MissionGraphNode_Editor" ,meta = (ClampMin = "0",EditCondition = "ChildrenLimitType == ENodeLimit::Limited", EditConditionHides))
	int32 ChildrenLimit;
	
#endif

#if WITH_EDITOR
	virtual bool IsNameEditable() const;

	virtual FLinearColor GetBackgroundColor() const;

	virtual FText GetNodeTitle() const;

	virtual void SetNodeTitle(const FText& NewTitle);

	virtual bool CanCreateConnection(UMissionGraphNode* Other, FText& ErrorMessage);

	virtual bool CanCreateConnectionTo(UMissionGraphNode* Other, int32 NumberOfChildrenNodes, FText& ErrorMessage);
	virtual bool CanCreateConnectionFrom(UMissionGraphNode* Other, int32 NumberOfParentNodes, FText& ErrorMessage);
#endif
};
