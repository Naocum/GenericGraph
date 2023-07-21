#pragma once

#include "CoreMinimal.h"
#include "MissionGraphNode.h"
#include "MissionGraphEdge.generated.h"

class UMissionGraph;

UCLASS(Blueprintable)
class MISSIONGRAPHRUNTIME_API UGenericGraphEdge : public UObject
{
	GENERATED_BODY()

public:
	UMissionGraphEdge();
	virtual ~UMissionGraphEdge();

	UPROPERTY(VisibleAnywhere, Category = "MissionGraphNode")
	UMissionGraph* Graph;

	UPROPERTY(BlueprintReadOnly, Category = "MissionGraphEdge")
	UMissionGraphNode* StartNode;

	UPROPERTY(BlueprintReadOnly, Category = "MissionGraphEdge")
	UMissionGraphNode* EndNode;

	UFUNCTION(BlueprintPure, Category = "MissionGraphEdge")
	UMissionGraph* GetGraph() const;

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditDefaultsOnly, Category = "MissionGraphNode_Editor")
	bool bShouldDrawTitle = false;

	UPROPERTY(EditDefaultsOnly, Category = "MissionGraphNode_Editor")
	FText NodeTitle;

	UPROPERTY(EditDefaultsOnly, Category = "MissionGraphEdge")
	FLinearColor EdgeColour = FLinearColor(0.9f, 0.9f, 0.9f, 1.0f);
#endif

#if WITH_EDITOR
	virtual FText GetNodeTitle() const { return NodeTitle; }
	FLinearColor GetEdgeColour() { return EdgeColour; }

	virtual void SetNodeTitle(const FText& NewTitle);
#endif
	
};
