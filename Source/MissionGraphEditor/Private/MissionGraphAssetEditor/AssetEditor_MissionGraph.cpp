#include "MissionGraphAssetEditor/AssetEditor_MissionGraph.h"
#include "MissionGraphEditorPCH.h"
#include "MissionGraphAssetEditor/AssetEditorToolbar_MissionGraph.h"
#include "MissionGraphAssetEditor/AssetGraphSchema_MissionGraph.h"
#include "MissionGraphAssetEditor/EditorCommands_MissionGraph.h"
#include "MissionGraphAssetEditor/EdGraph_MissionGraph.h"
#include "AssetToolsModule.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Framework/Commands/MissionCommands.h"
#include "GraphEditorActions.h"
#include "IDetailsView.h"
#include "PropertyEditorModule.h"
#include "Editor/UnrealEd/Public/Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "EdGraphUtilities.h"
#include "MissionGraphAssetEditor/EdGraph_MissionGraph.h"
#include "MissionGraphAssetEditor/EdNode_MissionGraphNode.h"
#include "MissionGraphAssetEditor/EdNode_MissionGraphEdge.h"
#include "AutoLayout/TreeLayoutStrategy.h"
#include "AutoLayout/ForceDirectedLayoutStrategy.h"

#define LOCTEXT_NAMESPACE "AssetEditor_MissionGraph"

const FName MissionGraphEditorAppName = FName(TEXT("MissionGraphEditorApp"));

struct FMissionGraphAssetEditorTabs
{
	// Tab identifiers
	static const FName MissionGraphPropertyID;
	static const FName ViewportID;
	static const FName MissionGraphEditorSettingsID;
};

//////////////////////////////////////////////////////////////////////////

const FName FMissionGraphAssetEditorTabs::MissionGraphPropertyID(TEXT("MissionGraphProperty"));
const FName FMissionGraphAssetEditorTabs::ViewportID(TEXT("Viewport"));
const FName FMissionGraphAssetEditorTabs::MissionGraphEditorSettingsID(TEXT("MissionGraphEditorSettings"));

//////////////////////////////////////////////////////////////////////////

FAssetEditor_MissionGraph::FAssetEditor_MissionGraph()
{
	EditingGraph = nullptr;

	GenricGraphEditorSettings = NewObject<UMissionGraphEditorSettings>(UMissionGraphEditorSettings::StaticClass());

#if ENGINE_MAJOR_VERSION < 5
	OnPackageSavedDelegateHandle = UPackage::PackageSavedEvent.AddRaw(this, &FAssetEditor_MissionGraph::OnPackageSaved);
#else // #if ENGINE_MAJOR_VERSION < 5
	OnPackageSavedDelegateHandle = UPackage::PackageSavedWithContextEvent.AddRaw(this, &FAssetEditor_MissionGraph::OnPackageSavedWithContext);
#endif // #else // #if ENGINE_MAJOR_VERSION < 5
}

FAssetEditor_MissionGraph::~FAssetEditor_MissionGraph()
{
#if ENGINE_MAJOR_VERSION < 5
	UPackage::PackageSavedEvent.Remove(OnPackageSavedDelegateHandle);
#else // #if ENGINE_MAJOR_VERSION < 5
	UPackage::PackageSavedWithContextEvent.Remove(OnPackageSavedDelegateHandle);
#endif // #else // #if ENGINE_MAJOR_VERSION < 5
}

void FAssetEditor_MissionGraph::InitMissionGraphAssetEditor(const EToolkitMode::Type Mode, const TSharedPtr< IToolkitHost >& InitToolkitHost, UMissionGraph* Graph)
{
	EditingGraph = Graph;
	CreateEdGraph();

	FMissionCommands::Register();
	FGraphEditorCommands::Register();
	FEditorCommands_MissionGraph::Register();

	if (!ToolbarBuilder.IsValid())
	{
		ToolbarBuilder = MakeShareable(new FAssetEditorToolbar_MissionGraph(SharedThis(this)));
	}

	BindCommands();

	CreateInternalWidgets();

	TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);

	ToolbarBuilder->AddMissionGraphToolbar(ToolbarExtender);

	// Layout
	const TSharedRef<FTabManager::FLayout> StandaloneDefaultLayout = FTabManager::NewLayout("Standalone_MissionGraphEditor_Layout_v1")
		->AddArea
		(
			FTabManager::NewPrimaryArea()->SetOrientation(Orient_Vertical)
#if ENGINE_MAJOR_VERSION < 5
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.1f)
				->AddTab(GetToolbarTabId(), ETabState::OpenedTab)->SetHideTabWell(true)
			)
#endif // #if ENGINE_MAJOR_VERSION < 5
			->Split
			(
				FTabManager::NewSplitter()->SetOrientation(Orient_Horizontal)->SetSizeCoefficient(0.9f)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.65f)
					->AddTab(FMissionGraphAssetEditorTabs::ViewportID, ETabState::OpenedTab)->SetHideTabWell(true)
				)
				->Split
				(
					FTabManager::NewSplitter()->SetOrientation(Orient_Vertical)
					->Split
					(
						FTabManager::NewStack()
						->SetSizeCoefficient(0.7f)
						->AddTab(FMissionGraphAssetEditorTabs::MissionGraphPropertyID, ETabState::OpenedTab)->SetHideTabWell(true)
					)
					->Split
					(
						FTabManager::NewStack()
						->SetSizeCoefficient(0.3f)
						->AddTab(FMissionGraphAssetEditorTabs::MissionGraphEditorSettingsID, ETabState::OpenedTab)
					)
				)
			)
		);

	const bool bCreateDefaultStandaloneMenu = true;
	const bool bCreateDefaultToolbar = true;
	FAssetEditorToolkit::InitAssetEditor(Mode, InitToolkitHost, MissionGraphEditorAppName, StandaloneDefaultLayout, bCreateDefaultStandaloneMenu, bCreateDefaultToolbar, EditingGraph, false);

	RegenerateMenusAndToolbars();
}

void FAssetEditor_MissionGraph::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	WorkspaceMenuCategory = InTabManager->AddLocalWorkspaceMenuCategory(LOCTEXT("WorkspaceMenu_MissionGraphEditor", "Mission Graph Editor"));
	auto WorkspaceMenuCategoryRef = WorkspaceMenuCategory.ToSharedRef();

	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

	InTabManager->RegisterTabSpawner(FMissionGraphAssetEditorTabs::ViewportID, FOnSpawnTab::CreateSP(this, &FAssetEditor_MissionGraph::SpawnTab_Viewport))
		.SetDisplayName(LOCTEXT("GraphCanvasTab", "Viewport"))
		.SetGroup(WorkspaceMenuCategoryRef)
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "GraphEditor.EventGraph_16x"));

	InTabManager->RegisterTabSpawner(FMissionGraphAssetEditorTabs::MissionGraphPropertyID, FOnSpawnTab::CreateSP(this, &FAssetEditor_MissionGraph::SpawnTab_Details))
		.SetDisplayName(LOCTEXT("DetailsTab", "Property"))
		.SetGroup(WorkspaceMenuCategoryRef)
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details"));

	InTabManager->RegisterTabSpawner(FMissionGraphAssetEditorTabs::MissionGraphEditorSettingsID, FOnSpawnTab::CreateSP(this, &FAssetEditor_MissionGraph::SpawnTab_EditorSettings))
		.SetDisplayName(LOCTEXT("EditorSettingsTab", "Mission Graph Editor Setttings"))
		.SetGroup(WorkspaceMenuCategoryRef)
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details"));
}

void FAssetEditor_MissionGraph::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);

	InTabManager->UnregisterTabSpawner(FMissionGraphAssetEditorTabs::ViewportID);
	InTabManager->UnregisterTabSpawner(FMissionGraphAssetEditorTabs::MissionGraphPropertyID);
	InTabManager->UnregisterTabSpawner(FMissionGraphAssetEditorTabs::MissionGraphEditorSettingsID);
}

FName FAssetEditor_MissionGraph::GetToolkitFName() const
{
	return FName("FMissionGraphEditor");
}

FText FAssetEditor_MissionGraph::GetBaseToolkitName() const
{
	return LOCTEXT("MissionGraphEditorAppLabel", "Mission Graph Editor");
}

FText FAssetEditor_MissionGraph::GetToolkitName() const
{
	const bool bDirtyState = EditingGraph->GetOutermost()->IsDirty();

	FFormatNamedArguments Args;
	Args.Add(TEXT("MissionGraphName"), FText::FromString(EditingGraph->GetName()));
	Args.Add(TEXT("DirtyState"), bDirtyState ? FText::FromString(TEXT("*")) : FText::GetEmpty());
	return FText::Format(LOCTEXT("MissionGraphEditorToolkitName", "{MissionGraphName}{DirtyState}"), Args);
}

FText FAssetEditor_MissionGraph::GetToolkitToolTipText() const
{
	return FAssetEditorToolkit::GetToolTipTextForObject(EditingGraph);
}

FLinearColor FAssetEditor_MissionGraph::GetWorldCentricTabColorScale() const
{
	return FLinearColor::White;
}

FString FAssetEditor_MissionGraph::GetWorldCentricTabPrefix() const
{
	return TEXT("MissionGraphEditor");
}

FString FAssetEditor_MissionGraph::GetDocumentationLink() const
{
	return TEXT("");
}

void FAssetEditor_MissionGraph::SaveAsset_Execute()
{
	if (EditingGraph != nullptr)
	{
		RebuildMissionGraph();
	}

	FAssetEditorToolkit::SaveAsset_Execute();
}

void FAssetEditor_MissionGraph::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(EditingGraph);
	Collector.AddReferencedObject(EditingGraph->EdGraph);
}

UMissionGraphEditorSettings* FAssetEditor_MissionGraph::GetSettings() const
{
	return GenricGraphEditorSettings;
}

TSharedRef<SDockTab> FAssetEditor_MissionGraph::SpawnTab_Viewport(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId() == FMissionGraphAssetEditorTabs::ViewportID);

	TSharedRef<SDockTab> SpawnedTab = SNew(SDockTab)
		.Label(LOCTEXT("ViewportTab_Title", "Viewport"));

	if (ViewportWidget.IsValid())
	{
		SpawnedTab->SetContent(ViewportWidget.ToSharedRef());
	}

	return SpawnedTab;
}

TSharedRef<SDockTab> FAssetEditor_MissionGraph::SpawnTab_Details(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId() == FMissionGraphAssetEditorTabs::MissionGraphPropertyID);

	return SNew(SDockTab)
#if ENGINE_MAJOR_VERSION < 5
		.Icon(FAppStyle::GetBrush("LevelEditor.Tabs.Details"))
#endif // #if ENGINE_MAJOR_VERSION < 5
		.Label(LOCTEXT("Details_Title", "Property"))
		[
			PropertyWidget.ToSharedRef()
		];
}

TSharedRef<SDockTab> FAssetEditor_MissionGraph::SpawnTab_EditorSettings(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId() == FMissionGraphAssetEditorTabs::MissionGraphEditorSettingsID);

	return SNew(SDockTab)
#if ENGINE_MAJOR_VERSION < 5
		.Icon(FAppStyle::GetBrush("LevelEditor.Tabs.Details"))
#endif // #if ENGINE_MAJOR_VERSION < 5
		.Label(LOCTEXT("EditorSettings_Title", "Mission Graph Editor Setttings"))
		[
			EditorSettingsWidget.ToSharedRef()
		];
}

void FAssetEditor_MissionGraph::CreateInternalWidgets()
{
	ViewportWidget = CreateViewportWidget();

	FDetailsViewArgs Args;
	Args.bHideSelectionTip = true;
	Args.NotifyHook = this;

	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyWidget = PropertyModule.CreateDetailView(Args);
	PropertyWidget->SetObject(EditingGraph);
	PropertyWidget->OnFinishedChangingProperties().AddSP(this, &FAssetEditor_MissionGraph::OnFinishedChangingProperties);

	EditorSettingsWidget = PropertyModule.CreateDetailView(Args);
	EditorSettingsWidget->SetObject(GenricGraphEditorSettings);
}

TSharedRef<SGraphEditor> FAssetEditor_MissionGraph::CreateViewportWidget()
{
	FGraphAppearanceInfo AppearanceInfo;
	AppearanceInfo.CornerText = LOCTEXT("AppearanceCornerText_MissionGraph", "Mission Graph");

	CreateCommandList();

	SGraphEditor::FGraphEditorEvents InEvents;
	InEvents.OnSelectionChanged = SGraphEditor::FOnSelectionChanged::CreateSP(this, &FAssetEditor_MissionGraph::OnSelectedNodesChanged);
	InEvents.OnNodeDoubleClicked = FSingleNodeEvent::CreateSP(this, &FAssetEditor_MissionGraph::OnNodeDoubleClicked);

	return SNew(SGraphEditor)
		.AdditionalCommands(GraphEditorCommands)
		.IsEditable(true)
		.Appearance(AppearanceInfo)
		.GraphToEdit(EditingGraph->EdGraph)
		.GraphEvents(InEvents)
		.AutoExpandActionMenu(true)
		.ShowGraphStateOverlay(false);
}

void FAssetEditor_MissionGraph::BindCommands()
{
	ToolkitCommands->MapAction(FEditorCommands_MissionGraph::Get().GraphSettings,
		FExecuteAction::CreateSP(this, &FAssetEditor_MissionGraph::GraphSettings),
		FCanExecuteAction::CreateSP(this, &FAssetEditor_MissionGraph::CanGraphSettings)
	);

	ToolkitCommands->MapAction(FEditorCommands_MissionGraph::Get().AutoArrange,
		FExecuteAction::CreateSP(this, &FAssetEditor_MissionGraph::AutoArrange),
		FCanExecuteAction::CreateSP(this, &FAssetEditor_MissionGraph::CanAutoArrange)
	);
}

void FAssetEditor_MissionGraph::CreateEdGraph()
{
	if (EditingGraph->EdGraph == nullptr)
	{
		EditingGraph->EdGraph = CastChecked<UEdGraph_MissionGraph>(FBlueprintEditorUtils::CreateNewGraph(EditingGraph, NAME_None, UEdGraph_MissionGraph::StaticClass(), UAssetGraphSchema_MissionGraph::StaticClass()));
		EditingGraph->EdGraph->bAllowDeletion = false;

		// Give the schema a chance to fill out any required nodes (like the results node)
		const UEdGraphSchema* Schema = EditingGraph->EdGraph->GetSchema();
		Schema->CreateDefaultNodesForGraph(*EditingGraph->EdGraph);
	}
}

void FAssetEditor_MissionGraph::CreateCommandList()
{
	if (GraphEditorCommands.IsValid())
	{
		return;
	}

	GraphEditorCommands = MakeShareable(new FUICommandList);

	// Can't use CreateSP here because derived editor are already implementing TSharedFromThis<FAssetEditorToolkit>
	// however it should be safe, since commands are being used only within this editor
	// if it ever crashes, this function will have to go away and be reimplemented in each derived class

	GraphEditorCommands->MapAction(FEditorCommands_MissionGraph::Get().GraphSettings,
		FExecuteAction::CreateRaw(this, &FAssetEditor_MissionGraph::GraphSettings),
		FCanExecuteAction::CreateRaw(this, &FAssetEditor_MissionGraph::CanGraphSettings));

	GraphEditorCommands->MapAction(FEditorCommands_MissionGraph::Get().AutoArrange,
		FExecuteAction::CreateRaw(this, &FAssetEditor_MissionGraph::AutoArrange),
		FCanExecuteAction::CreateRaw(this, &FAssetEditor_MissionGraph::CanAutoArrange));

	GraphEditorCommands->MapAction(FMissionCommands::Get().SelectAll,
		FExecuteAction::CreateRaw(this, &FAssetEditor_MissionGraph::SelectAllNodes),
		FCanExecuteAction::CreateRaw(this, &FAssetEditor_MissionGraph::CanSelectAllNodes)
	);

	GraphEditorCommands->MapAction(FMissionCommands::Get().Delete,
		FExecuteAction::CreateRaw(this, &FAssetEditor_MissionGraph::DeleteSelectedNodes),
		FCanExecuteAction::CreateRaw(this, &FAssetEditor_MissionGraph::CanDeleteNodes)
	);

	GraphEditorCommands->MapAction(FMissionCommands::Get().Copy,
		FExecuteAction::CreateRaw(this, &FAssetEditor_MissionGraph::CopySelectedNodes),
		FCanExecuteAction::CreateRaw(this, &FAssetEditor_MissionGraph::CanCopyNodes)
	);

	GraphEditorCommands->MapAction(FMissionCommands::Get().Cut,
		FExecuteAction::CreateRaw(this, &FAssetEditor_MissionGraph::CutSelectedNodes),
		FCanExecuteAction::CreateRaw(this, &FAssetEditor_MissionGraph::CanCutNodes)
	);

	GraphEditorCommands->MapAction(FMissionCommands::Get().Paste,
		FExecuteAction::CreateRaw(this, &FAssetEditor_MissionGraph::PasteNodes),
		FCanExecuteAction::CreateRaw(this, &FAssetEditor_MissionGraph::CanPasteNodes)
	);

	GraphEditorCommands->MapAction(FMissionCommands::Get().Duplicate,
		FExecuteAction::CreateRaw(this, &FAssetEditor_MissionGraph::DuplicateNodes),
		FCanExecuteAction::CreateRaw(this, &FAssetEditor_MissionGraph::CanDuplicateNodes)
	);

	GraphEditorCommands->MapAction(FMissionCommands::Get().Rename,
		FExecuteAction::CreateSP(this, &FAssetEditor_MissionGraph::OnRenameNode),
		FCanExecuteAction::CreateSP(this, &FAssetEditor_MissionGraph::CanRenameNodes)
	);
}

TSharedPtr<SGraphEditor> FAssetEditor_MissionGraph::GetCurrGraphEditor() const
{
	return ViewportWidget;
}

FGraphPanelSelectionSet FAssetEditor_MissionGraph::GetSelectedNodes() const
{
	FGraphPanelSelectionSet CurrentSelection;
	TSharedPtr<SGraphEditor> FocusedGraphEd = GetCurrGraphEditor();
	if (FocusedGraphEd.IsValid())
	{
		CurrentSelection = FocusedGraphEd->GetSelectedNodes();
	}

	return CurrentSelection;
}

void FAssetEditor_MissionGraph::RebuildMissionGraph()
{
	if (EditingGraph == nullptr)
	{
		LOG_WARNING(TEXT("FMissionGraphAssetEditor::RebuildMissionGraph EditingGraph is nullptr"));
		return;
	}

	UEdGraph_MissionGraph* EdGraph = Cast<UEdGraph_MissionGraph>(EditingGraph->EdGraph);
	check(EdGraph != nullptr);

	EdGraph->RebuildMissionGraph();
}

void FAssetEditor_MissionGraph::SelectAllNodes()
{
	TSharedPtr<SGraphEditor> CurrentGraphEditor = GetCurrGraphEditor();
	if (CurrentGraphEditor.IsValid())
	{
		CurrentGraphEditor->SelectAllNodes();
	}
}

bool FAssetEditor_MissionGraph::CanSelectAllNodes()
{
	return true;
}

void FAssetEditor_MissionGraph::DeleteSelectedNodes()
{
	TSharedPtr<SGraphEditor> CurrentGraphEditor = GetCurrGraphEditor();
	if (!CurrentGraphEditor.IsValid())
	{
		return;
	}

	const FScopedTransaction Transaction(FMissionCommands::Get().Delete->GetDescription());

	CurrentGraphEditor->GetCurrentGraph()->Modify();

	const FGraphPanelSelectionSet SelectedNodes = CurrentGraphEditor->GetSelectedNodes();
	CurrentGraphEditor->ClearSelectionSet();

	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		UEdGraphNode* EdNode = Cast<UEdGraphNode>(*NodeIt);
		if (EdNode == nullptr || !EdNode->CanUserDeleteNode())
			continue;;

		if (UEdNode_MissionGraphNode* EdNode_Node = Cast<UEdNode_MissionGraphNode>(EdNode))
		{
			EdNode_Node->Modify();

			const UEdGraphSchema* Schema = EdNode_Node->GetSchema();
			if (Schema != nullptr)
			{
				Schema->BreakNodeLinks(*EdNode_Node);
			}

			EdNode_Node->DestroyNode();
		}
		else
		{
			EdNode->Modify();
			EdNode->DestroyNode();
		}
	}
}

bool FAssetEditor_MissionGraph::CanDeleteNodes()
{
	// If any of the nodes can be deleted then we should allow deleting
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator SelectedIter(SelectedNodes); SelectedIter; ++SelectedIter)
	{
		UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter);
		if (Node != nullptr && Node->CanUserDeleteNode())
		{
			return true;
		}
	}

	return false;
}

void FAssetEditor_MissionGraph::DeleteSelectedDuplicatableNodes()
{
	TSharedPtr<SGraphEditor> CurrentGraphEditor = GetCurrGraphEditor();
	if (!CurrentGraphEditor.IsValid())
	{
		return;
	}

	const FGraphPanelSelectionSet OldSelectedNodes = CurrentGraphEditor->GetSelectedNodes();
	CurrentGraphEditor->ClearSelectionSet();

	for (FGraphPanelSelectionSet::TConstIterator SelectedIter(OldSelectedNodes); SelectedIter; ++SelectedIter)
	{
		UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter);
		if (Node && Node->CanDuplicateNode())
		{
			CurrentGraphEditor->SetNodeSelection(Node, true);
		}
	}

	// Delete the duplicatable nodes
	DeleteSelectedNodes();

	CurrentGraphEditor->ClearSelectionSet();

	for (FGraphPanelSelectionSet::TConstIterator SelectedIter(OldSelectedNodes); SelectedIter; ++SelectedIter)
	{
		if (UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter))
		{
			CurrentGraphEditor->SetNodeSelection(Node, true);
		}
	}
}

void FAssetEditor_MissionGraph::CutSelectedNodes()
{
	CopySelectedNodes();
	DeleteSelectedDuplicatableNodes();
}

bool FAssetEditor_MissionGraph::CanCutNodes()
{
	return CanCopyNodes() && CanDeleteNodes();
}

void FAssetEditor_MissionGraph::CopySelectedNodes()
{
	// Export the selected nodes and place the text on the clipboard
	FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();

	FString ExportedText;

	for (FGraphPanelSelectionSet::TIterator SelectedIter(SelectedNodes); SelectedIter; ++SelectedIter)
	{
		UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter);
		if (Node == nullptr)
		{
			SelectedIter.RemoveCurrent();
			continue;
		}

		if (UEdNode_MissionGraphEdge* EdNode_Edge = Cast<UEdNode_MissionGraphEdge>(*SelectedIter))
		{
			UEdNode_MissionGraphNode* StartNode = EdNode_Edge->GetStartNode();
			UEdNode_MissionGraphNode* EndNode = EdNode_Edge->GetEndNode();

			if (!SelectedNodes.Contains(StartNode) || !SelectedNodes.Contains(EndNode))
			{
				SelectedIter.RemoveCurrent();
				continue;
			}
		}

		Node->PrepareForCopying();
	}

	FEdGraphUtilities::ExportNodesToText(SelectedNodes, ExportedText);
	FPlatformApplicationMisc::ClipboardCopy(*ExportedText);
}

bool FAssetEditor_MissionGraph::CanCopyNodes()
{
	// If any of the nodes can be duplicated then we should allow copying
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator SelectedIter(SelectedNodes); SelectedIter; ++SelectedIter)
	{
		UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter);
		if (Node && Node->CanDuplicateNode())
		{
			return true;
		}
	}

	return false;
}

void FAssetEditor_MissionGraph::PasteNodes()
{
	TSharedPtr<SGraphEditor> CurrentGraphEditor = GetCurrGraphEditor();
	if (CurrentGraphEditor.IsValid())
	{
		PasteNodesHere(CurrentGraphEditor->GetPasteLocation());
	}
}

void FAssetEditor_MissionGraph::PasteNodesHere(const FVector2D& Location)
{
	// Find the graph editor with focus
	TSharedPtr<SGraphEditor> CurrentGraphEditor = GetCurrGraphEditor();
	if (!CurrentGraphEditor.IsValid())
	{
		return;
	}
	// Select the newly pasted stuff
	UEdGraph* EdGraph = CurrentGraphEditor->GetCurrentGraph();

	{
		const FScopedTransaction Transaction(FMissionCommands::Get().Paste->GetDescription());
		EdGraph->Modify();

		// Clear the selection set (newly pasted stuff will be selected)
		CurrentGraphEditor->ClearSelectionSet();

		// Grab the text to paste from the clipboard.
		FString TextToImport;
		FPlatformApplicationMisc::ClipboardPaste(TextToImport);

		// Import the nodes
		TSet<UEdGraphNode*> PastedNodes;
		FEdGraphUtilities::ImportNodesFromText(EdGraph, TextToImport, PastedNodes);

		//Average position of nodes so we can move them while still maintaining relative distances to each other
		FVector2D AvgNodePosition(0.0f, 0.0f);

		for (TSet<UEdGraphNode*>::TIterator It(PastedNodes); It; ++It)
		{
			UEdGraphNode* Node = *It;
			AvgNodePosition.X += Node->NodePosX;
			AvgNodePosition.Y += Node->NodePosY;
		}

		float InvNumNodes = 1.0f / float(PastedNodes.Num());
		AvgNodePosition.X *= InvNumNodes;
		AvgNodePosition.Y *= InvNumNodes;

		for (TSet<UEdGraphNode*>::TIterator It(PastedNodes); It; ++It)
		{
			UEdGraphNode* Node = *It;
			CurrentGraphEditor->SetNodeSelection(Node, true);

			Node->NodePosX = (Node->NodePosX - AvgNodePosition.X) + Location.X;
			Node->NodePosY = (Node->NodePosY - AvgNodePosition.Y) + Location.Y;

			Node->SnapToGrid(16);

			// Give new node a different Guid from the old one
			Node->CreateNewGuid();
		}
	}

	// Update UI
	CurrentGraphEditor->NotifyGraphChanged();

	UObject* GraphOwner = EdGraph->GetOuter();
	if (GraphOwner)
	{
		GraphOwner->PostEditChange();
		GraphOwner->MarkPackageDirty();
	}
}

bool FAssetEditor_MissionGraph::CanPasteNodes()
{
	TSharedPtr<SGraphEditor> CurrentGraphEditor = GetCurrGraphEditor();
	if (!CurrentGraphEditor.IsValid())
	{
		return false;
	}

	FString ClipboardContent;
	FPlatformApplicationMisc::ClipboardPaste(ClipboardContent);

	return FEdGraphUtilities::CanImportNodesFromText(CurrentGraphEditor->GetCurrentGraph(), ClipboardContent);
}

void FAssetEditor_MissionGraph::DuplicateNodes()
{
	CopySelectedNodes();
	PasteNodes();
}

bool FAssetEditor_MissionGraph::CanDuplicateNodes()
{
	return CanCopyNodes();
}

void FAssetEditor_MissionGraph::GraphSettings()
{
	PropertyWidget->SetObject(EditingGraph);
}

bool FAssetEditor_MissionGraph::CanGraphSettings() const
{
	return true;
}

void FAssetEditor_MissionGraph::AutoArrange()
{
	UEdGraph_MissionGraph* EdGraph = Cast<UEdGraph_MissionGraph>(EditingGraph->EdGraph);
	check(EdGraph != nullptr);

	const FScopedTransaction Transaction(LOCTEXT("MissionGraphEditorAutoArrange", "Mission Graph Editor: Auto Arrange"));

	EdGraph->Modify();

	UAutoLayoutStrategy* LayoutStrategy = nullptr;
	switch (GenricGraphEditorSettings->AutoLayoutStrategy)
	{
	case EAutoLayoutStrategy::Tree:
		LayoutStrategy = NewObject<UAutoLayoutStrategy>(EdGraph, UTreeLayoutStrategy::StaticClass());
		break;
	case EAutoLayoutStrategy::ForceDirected:
		LayoutStrategy = NewObject<UAutoLayoutStrategy>(EdGraph, UForceDirectedLayoutStrategy::StaticClass());
		break;
	default:
		break;
	}

	if (LayoutStrategy != nullptr)
	{
		LayoutStrategy->Settings = GenricGraphEditorSettings;
		LayoutStrategy->Layout(EdGraph);
		LayoutStrategy->ConditionalBeginDestroy();
	}
	else
	{
		LOG_ERROR(TEXT("FAssetEditor_MissionGraph::AutoArrange LayoutStrategy is null."));
	}
}

bool FAssetEditor_MissionGraph::CanAutoArrange() const
{
	return EditingGraph != nullptr && Cast<UEdGraph_MissionGraph>(EditingGraph->EdGraph) != nullptr;
}

void FAssetEditor_MissionGraph::OnRenameNode()
{
	TSharedPtr<SGraphEditor> CurrentGraphEditor = GetCurrGraphEditor();
	if (CurrentGraphEditor.IsValid())
	{
		const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
		for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
		{
			UEdGraphNode* SelectedNode = Cast<UEdGraphNode>(*NodeIt);
			if (SelectedNode != NULL && SelectedNode->bCanRenameNode)
			{
				CurrentGraphEditor->IsNodeTitleVisible(SelectedNode, true);
				break;
			}
		}
	}
}

bool FAssetEditor_MissionGraph::CanRenameNodes() const
{
	UEdGraph_MissionGraph* EdGraph = Cast<UEdGraph_MissionGraph>(EditingGraph->EdGraph);
	check(EdGraph != nullptr);

	UMissionGraph* Graph = EdGraph->GetMissionGraph();
	check(Graph != nullptr)

	return Graph->bCanRenameNode && GetSelectedNodes().Num() == 1;
}

void FAssetEditor_MissionGraph::OnSelectedNodesChanged(const TSet<class UObject*>& NewSelection)
{
	TArray<UObject*> Selection;

	for (UObject* SelectionEntry : NewSelection)
	{
		Selection.Add(SelectionEntry);
	}

	if (Selection.Num() == 0) 
	{
		PropertyWidget->SetObject(EditingGraph);

	}
	else
	{
		PropertyWidget->SetObjects(Selection);
	}
}

void FAssetEditor_MissionGraph::OnNodeDoubleClicked(UEdGraphNode* Node)
{
	
}

void FAssetEditor_MissionGraph::OnFinishedChangingProperties(const FPropertyChangedEvent& PropertyChangedEvent)
{
	if (EditingGraph == nullptr)
		return;

	EditingGraph->EdGraph->GetSchema()->ForceVisualizationCacheClear();
}

#if ENGINE_MAJOR_VERSION < 5
void FAssetEditor_MissionGraph::OnPackageSaved(const FString& PackageFileName, UObject* Outer)
{
	RebuildMissionGraph();
}
#else // #if ENGINE_MAJOR_VERSION < 5
void FAssetEditor_MissionGraph::OnPackageSavedWithContext(const FString& PackageFileName, UPackage* Package, FObjectPostSaveContext ObjectSaveContext)
{
	RebuildMissionGraph();
}
#endif // #else // #if ENGINE_MAJOR_VERSION < 5

void FAssetEditor_MissionGraph::RegisterToolbarTab(const TSharedRef<class FTabManager>& InTabManager) 
{
	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);
}


#undef LOCTEXT_NAMESPACE

