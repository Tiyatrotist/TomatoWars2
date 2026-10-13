#include "BPGraphToolLibrary.h"

#include "BaseWidgetBlueprint.h"
#include "Components/Widget.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraphUtilities.h"
#include "Engine/Blueprint.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/CompilerResultsLog.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Logging/TokenizedMessage.h"
#include "Misc/FileHelper.h"
#include "UObject/TopLevelAssetPath.h"

namespace
{
	UEdGraph* FindGraph(UBlueprint* Blueprint, const FString& GraphName)
	{
		if (!Blueprint)
		{
			return nullptr;
		}

		TArray<UEdGraph*> AllGraphs;
		Blueprint->GetAllGraphs(AllGraphs);
		for (UEdGraph* Graph : AllGraphs)
		{
			if (Graph && Graph->GetName().Equals(GraphName, ESearchCase::IgnoreCase))
			{
				return Graph;
			}
		}
		return nullptr;
	}
}

TArray<FString> UBPGraphToolLibrary::ListGraphs(UBlueprint* Blueprint)
{
	TArray<FString> Names;
	if (!Blueprint)
	{
		return Names;
	}

	TArray<UEdGraph*> AllGraphs;
	Blueprint->GetAllGraphs(AllGraphs);
	for (const UEdGraph* Graph : AllGraphs)
	{
		if (Graph)
		{
			Names.Add(FString::Printf(TEXT("%s (%d node)"), *Graph->GetName(), Graph->Nodes.Num()));
		}
	}
	return Names;
}

FString UBPGraphToolLibrary::ExportGraph(UBlueprint* Blueprint, const FString& GraphName)
{
	UEdGraph* Graph = FindGraph(Blueprint, GraphName);
	if (!Graph)
	{
		return FString();
	}

	TSet<UObject*> NodesToExport;
	for (UEdGraphNode* Node : Graph->Nodes)
	{
		if (Node)
		{
			NodesToExport.Add(Node);
		}
	}

	FString ExportedText;
	FEdGraphUtilities::ExportNodesToText(NodesToExport, ExportedText);
	return ExportedText;
}

bool UBPGraphToolLibrary::ExportGraphToFile(UBlueprint* Blueprint, const FString& GraphName, const FString& FilePath)
{
	const FString Text = ExportGraph(Blueprint, GraphName);
	if (Text.IsEmpty())
	{
		return false;
	}
	return FFileHelper::SaveStringToFile(Text, *FilePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}

int32 UBPGraphToolLibrary::ClearGraph(UBlueprint* Blueprint, const FString& GraphName)
{
	UEdGraph* Graph = FindGraph(Blueprint, GraphName);
	if (!Graph)
	{
		return -1;
	}

	Graph->Modify();

	// Silme sirasinda diziyi degistirdigimiz icin anlik kopya uzerinde geziyoruz.
	TArray<UEdGraphNode*> Snapshot = Graph->Nodes;
	int32 Removed = 0;
	for (UEdGraphNode* Node : Snapshot)
	{
		if (!Node)
		{
			continue;
		}
		Node->Modify();
		Graph->RemoveNode(Node, /*bBreakAllLinks=*/true);
		++Removed;
	}

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	return Removed;
}

int32 UBPGraphToolLibrary::ImportNodes(UBlueprint* Blueprint, const FString& GraphName, const FString& T3DText)
{
	UEdGraph* Graph = FindGraph(Blueprint, GraphName);
	if (!Graph)
	{
		return -1;
	}
	if (T3DText.IsEmpty() || !FEdGraphUtilities::CanImportNodesFromText(Graph, T3DText))
	{
		return -2;
	}

	Graph->Modify();
	TSet<UEdGraphNode*> ImportedNodes;
	FEdGraphUtilities::ImportNodesFromText(Graph, T3DText, ImportedNodes);
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	return ImportedNodes.Num();
}

int32 UBPGraphToolLibrary::ReplaceGraph(UBlueprint* Blueprint, const FString& GraphName, const FString& T3DText)
{
	UEdGraph* Graph = FindGraph(Blueprint, GraphName);
	if (!Graph)
	{
		return -1;
	}
	// Once metnin gecerli oldugunu dogrula; aksi halde graph'i bosaltmis oluruz.
	if (T3DText.IsEmpty() || !FEdGraphUtilities::CanImportNodesFromText(Graph, T3DText))
	{
		return -2;
	}

	ClearGraph(Blueprint, GraphName);
	return ImportNodes(Blueprint, GraphName, T3DText);
}

int32 UBPGraphToolLibrary::ReplaceGraphFromFile(UBlueprint* Blueprint, const FString& GraphName, const FString& FilePath)
{
	FString Text;
	if (!FFileHelper::LoadFileToString(Text, *FilePath))
	{
		return -3;
	}
	return ReplaceGraph(Blueprint, GraphName, Text);
}

int32 UBPGraphToolLibrary::ImportNodesFromFile(UBlueprint* Blueprint, const FString& GraphName, const FString& FilePath)
{
	FString Text;
	if (!FFileHelper::LoadFileToString(Text, *FilePath))
	{
		return -3;
	}
	return ImportNodes(Blueprint, GraphName, Text);
}

void UBPGraphToolLibrary::RegenerateSkeleton(UBlueprint* Blueprint)
{
	if (Blueprint)
	{
		FKismetEditorUtilities::CompileBlueprint(Blueprint, EBlueprintCompileOptions::RegenerateSkeletonOnly);
	}
}

void UBPGraphToolLibrary::RefreshAllNodes(UBlueprint* Blueprint)
{
	if (Blueprint)
	{
		FBlueprintEditorUtils::RefreshAllNodes(Blueprint);
	}
}

int32 UBPGraphToolLibrary::RefreshGraphNodes(UBlueprint* Blueprint, const FString& GraphName)
{
	UEdGraph* Graph = FindGraph(Blueprint, GraphName);
	if (!Graph)
	{
		return -1;
	}

	int32 Refreshed = 0;
	TArray<UEdGraphNode*> Snapshot = Graph->Nodes;
	for (UEdGraphNode* Node : Snapshot)
	{
		if (!Node)
		{
			continue;
		}
		Node->Modify();
		Node->ReconstructNode();
		++Refreshed;
	}

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	return Refreshed;
}

FString UBPGraphToolLibrary::ReplaceGraphFromFileAndFix(UBlueprint* Blueprint, const FString& GraphName, const FString& FilePath)
{
	const int32 NodeCount = ReplaceGraphFromFile(Blueprint, GraphName, FilePath);
	if (NodeCount < 0)
	{
		return FString::Printf(TEXT("HATA: replace basarisiz (kod %d)"), NodeCount);
	}

	// Skeleton'i uret ki graph'taki custom event'ler cozumlenebilsin,
	// ardindan node'lari yeniden kurup olu pinleri temizle.
	RegenerateSkeleton(Blueprint);
	RefreshAllNodes(Blueprint);

	const FString Report = CompileAndReport(Blueprint);
	return FString::Printf(TEXT("%d node | %s"), NodeCount, *Report);
}

bool UBPGraphToolLibrary::AddInterface(UBlueprint* Blueprint, const FString& InterfaceClassPath)
{
	if (!Blueprint)
	{
		return false;
	}

	const FTopLevelAssetPath Path(InterfaceClassPath);
	if (!Path.IsValid())
	{
		return false;
	}
	return FBlueprintEditorUtils::ImplementNewInterface(Blueprint, Path);
}

TArray<FString> UBPGraphToolLibrary::ListInterfaces(UBlueprint* Blueprint)
{
	TArray<FString> Paths;
	if (!Blueprint)
	{
		return Paths;
	}

	for (const FBPInterfaceDescription& Description : Blueprint->ImplementedInterfaces)
	{
		if (const UClass* InterfaceClass = Description.Interface.Get())
		{
			Paths.Add(InterfaceClass->GetPathName());
		}
	}
	return Paths;
}

FString UBPGraphToolLibrary::CompileAndReport(UBlueprint* Blueprint)
{
	if (!Blueprint)
	{
		return TEXT("HATA: Blueprint bulunamadi");
	}

	FCompilerResultsLog Results;
	Results.SetSilentMode(true);
	FKismetEditorUtilities::CompileBlueprint(Blueprint, EBlueprintCompileOptions::None, &Results);

	if (Results.NumErrors == 0 && Results.NumWarnings == 0)
	{
		return TEXT("OK");
	}

	FString Report = FString::Printf(TEXT("%d hata, %d uyari"), Results.NumErrors, Results.NumWarnings);
	for (const TSharedRef<FTokenizedMessage>& Message : Results.Messages)
	{
		Report += TEXT("\n  ") + Message->ToText().ToString();
	}
	return Report;
}

TArray<FString> UBPGraphToolLibrary::ListWidgets(UBlueprint* WidgetBlueprint)
{
	TArray<FString> Names;
	UBaseWidgetBlueprint* WidgetBP = Cast<UBaseWidgetBlueprint>(WidgetBlueprint);
	if (!WidgetBP)
	{
		return Names;
	}

	for (UWidget* Widget : WidgetBP->GetAllSourceWidgets())
	{
		if (Widget)
		{
			Names.Add(FString::Printf(TEXT("%s | %s | degisken=%s"),
				*Widget->GetName(),
				*Widget->GetClass()->GetName(),
				Widget->bIsVariable ? TEXT("evet") : TEXT("hayir")));
		}
	}
	return Names;
}

bool UBPGraphToolLibrary::SetWidgetIsVariable(UBlueprint* WidgetBlueprint, const FString& WidgetName, bool bIsVariable)
{
	UBaseWidgetBlueprint* WidgetBP = Cast<UBaseWidgetBlueprint>(WidgetBlueprint);
	if (!WidgetBP)
	{
		return false;
	}

	for (UWidget* Widget : WidgetBP->GetAllSourceWidgets())
	{
		if (Widget && Widget->GetName().Equals(WidgetName, ESearchCase::IgnoreCase))
		{
			Widget->Modify();
			Widget->bIsVariable = bIsVariable;
			FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(WidgetBlueprint);
			return true;
		}
	}
	return false;
}

bool UBPGraphToolLibrary::SetVariableReplicated(UBlueprint* Blueprint, FName VariableName, bool bReplicated)
{
	if (!Blueprint)
	{
		return false;
	}

	for (FBPVariableDescription& Variable : Blueprint->NewVariables)
	{
		if (Variable.VarName == VariableName)
		{
			Blueprint->Modify();
			if (bReplicated)
			{
				Variable.PropertyFlags |= CPF_Net;
			}
			else
			{
				Variable.PropertyFlags &= ~CPF_Net;
			}
			FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
			return true;
		}
	}
	return false;
}

TArray<FString> UBPGraphToolLibrary::ListVariables(UBlueprint* Blueprint)
{
	TArray<FString> Out;
	if (!Blueprint)
	{
		return Out;
	}

	for (const FBPVariableDescription& Variable : Blueprint->NewVariables)
	{
		Out.Add(FString::Printf(TEXT("%s | %s | replike=%s"),
			*Variable.VarName.ToString(),
			*Variable.VarType.PinCategory.ToString(),
			(Variable.PropertyFlags & CPF_Net) ? TEXT("evet") : TEXT("hayir")));
	}
	return Out;
}
