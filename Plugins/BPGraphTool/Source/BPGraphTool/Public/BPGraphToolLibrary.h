// Blueprint graph'lari programatik olarak okumayi/yazmayi saglayan editor araci.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "BPGraphToolLibrary.generated.h"

class UBlueprint;

/**
 * Unreal'in Python API'si node seviyesinde graph duzenlemeye izin vermez
 * (UEdGraph::Nodes korumalidir). Bu kutuphane gerekli editor fonksiyonlarini
 * Python'a acar: graph'i T3D metni olarak okuma/yazma, temizleme, arabirim
 * ekleme ve derleme raporu alma.
 */
UCLASS()
class UBPGraphToolLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Blueprint'teki tum graph isimlerini dondurur (EventGraph, fonksiyonlar, makrolar). */
	UFUNCTION(BlueprintCallable, Category = "BPGraphTool")
	static TArray<FString> ListGraphs(UBlueprint* Blueprint);

	/** Graph'in node'larini T3D (kopyala/yapistir) metni olarak dondurur. */
	UFUNCTION(BlueprintCallable, Category = "BPGraphTool")
	static FString ExportGraph(UBlueprint* Blueprint, const FString& GraphName);

	/** Graph'i T3D metni olarak dosyaya yazar. Basarili olursa true. */
	UFUNCTION(BlueprintCallable, Category = "BPGraphTool")
	static bool ExportGraphToFile(UBlueprint* Blueprint, const FString& GraphName, const FString& FilePath);

	/** Graph'taki tum node'lari siler, silinen sayiyi dondurur (-1: graph yok). */
	UFUNCTION(BlueprintCallable, Category = "BPGraphTool")
	static int32 ClearGraph(UBlueprint* Blueprint, const FString& GraphName);

	/** T3D metnindeki node'lari graph'a ekler (-1: graph yok, -2: metin uyumsuz). */
	UFUNCTION(BlueprintCallable, Category = "BPGraphTool")
	static int32 ImportNodes(UBlueprint* Blueprint, const FString& GraphName, const FString& T3DText);

	/** Graph'i temizleyip T3D metnini uygular. Eklenen node sayisini dondurur. */
	UFUNCTION(BlueprintCallable, Category = "BPGraphTool")
	static int32 ReplaceGraph(UBlueprint* Blueprint, const FString& GraphName, const FString& T3DText);

	/** ReplaceGraph'in dosyadan okuyan hali; buyuk graph'lar icin tercih edilir. */
	UFUNCTION(BlueprintCallable, Category = "BPGraphTool")
	static int32 ReplaceGraphFromFile(UBlueprint* Blueprint, const FString& GraphName, const FString& FilePath);

	/** ImportNodes'un dosyadan okuyan hali (graph temizlenmez). */
	UFUNCTION(BlueprintCallable, Category = "BPGraphTool")
	static int32 ImportNodesFromFile(UBlueprint* Blueprint, const FString& GraphName, const FString& FilePath);

	/** Yalnizca skeleton class'i yeniden uretir; custom event'ler boylece cagirilabilir hale gelir. */
	UFUNCTION(BlueprintCallable, Category = "BPGraphTool")
	static void RegenerateSkeleton(UBlueprint* Blueprint);

	/** Blueprint'in tum node'larini yeniden kurar (sag tik > Refresh Nodes karsiligi). */
	UFUNCTION(BlueprintCallable, Category = "BPGraphTool")
	static void RefreshAllNodes(UBlueprint* Blueprint);

	/** Tek bir graph'in node'larini yeniden kurar; yeniden kurulan sayiyi dondurur. */
	UFUNCTION(BlueprintCallable, Category = "BPGraphTool")
	static int32 RefreshGraphNodes(UBlueprint* Blueprint, const FString& GraphName);

	/**
	 * Graph'i dosyadan degistirir ve dogru sirayi uygular:
	 * temizle -> import -> skeleton uret -> node'lari yenile -> derle.
	 * Donus: "<node sayisi> node | <derleme raporu>"
	 */
	UFUNCTION(BlueprintCallable, Category = "BPGraphTool")
	static FString ReplaceGraphFromFileAndFix(UBlueprint* Blueprint, const FString& GraphName, const FString& FilePath);

	/**
	 * Blueprint'e arabirim ekler.
	 * @param InterfaceClassPath Orn. "/Game/Blueprints/1nteract.1nteract_C"
	 */
	UFUNCTION(BlueprintCallable, Category = "BPGraphTool")
	static bool AddInterface(UBlueprint* Blueprint, const FString& InterfaceClassPath);

	/** Blueprint'in uyguladigi arabirimleri listeler. */
	UFUNCTION(BlueprintCallable, Category = "BPGraphTool")
	static TArray<FString> ListInterfaces(UBlueprint* Blueprint);

	/** Blueprint'i derler ve "OK" ya da hata/uyari dokumunu dondurur. */
	UFUNCTION(BlueprintCallable, Category = "BPGraphTool")
	static FString CompileAndReport(UBlueprint* Blueprint);
};
