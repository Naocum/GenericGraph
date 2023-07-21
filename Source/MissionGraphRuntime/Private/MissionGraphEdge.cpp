#include "MissionGraphEdge.h"

UMissionGraphEdge::UMissionGraphEdge()
{

}

UMissionGraphEdge::~UMissionGraphEdge()
{

}

UMissionGraph* UMissionGraphEdge::GetGraph() const
{
	return Graph;
}

#if WITH_EDITOR
void UMissionGraphEdge::SetNodeTitle(const FText& NewTitle)
{
	NodeTitle = NewTitle;
}
#endif // #if WITH_EDITOR
