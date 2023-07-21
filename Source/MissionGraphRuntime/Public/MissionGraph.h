#pragma once

#include "CoreMinimal.h"
#include "MissionGraphNode.h"
#include "MissionGraphEdge.h"
#include "GameplayTagContainer.h"
#include "MissionGraph.generated.h"

UCLASS(Blueprintable)
    class MISSIONGRAPHRUNTIME_API UMissionGraph : public UObject
{
	GENERATED_BODY()

public:
	UMissionGraph();
	virtual ~UMissionGraph();

	UPROPERTY(EditDefaultsOnly, Category = "MissionGraph")
	FString Name;

	UPROPERTY(EditDefaultsOnly, Category = "MissionGraph")
	TSubclassOf<UMissionGraphNode> NodeType;

	UPROPERTY(EditDefaultsOnly, Category = "MissionGraph")
	TSubclassOf<UMissionGraphEdge> EdgeType;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MissionGraph")
	FGameplayTagContainer GraphTags;

	UPROPERTY(BlueprintReadOnly, Category = "MissionGraph")
	TArray<UMissionGraphNode*> RootNodes;

	UPROPERTY(BlueprintReadOnly, Category = "MissionGraph")
	TArray<UMissionGraphNode*> AllNodes;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MissionGraph")
	bool bEdgeEnabled;

	UFUNCTION(BlueprintCallable, Category = "MissionGraph")
	void Print(bool ToConsole = true, bool ToScreen = true);

	UFUNCTION(BlueprintCallable, Category = "MissionGraph")
	int GetLevelNum() const;

	UFUNCTION(BlueprintCallable, Category = "MissionGraph")
	void GetNodesByLevel(int Level, TArray<UMissionGraphNode*>& Nodes);

	void ClearGraph();

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	class UEdGraph* EdGraph;

	UPROPERTY(EditDefaultsOnly, Category = "MissionGraph_Editor")
	bool bCanRenameNode;

	UPROPERTY(EditDefaultsOnly, Category = "MissionGraph_Editor")
	bool bCanBeCyclical;
#endif
};
