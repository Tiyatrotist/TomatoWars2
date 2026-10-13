// Copyright 2025 Onur Altuntaş. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SWidget.h"

/**
 * NexxNote Eklentisi için ortak yardımcı fonksiyonlar
 */
namespace NexxNoteHelpers
{
    /**
     * Verilen kök widget'dan başlayarak tüm alt widget'ları rekursif olarak bulur
     */
    inline void FindAllWidgetsRecursive(TSharedRef<SWidget> Root, TArray<TSharedRef<SWidget>>& OutWidgets)
    {
        OutWidgets.Add(Root);
        
        FChildren* Children = Root->GetChildren();
        if (!Children)
        {
            return;
        }
        
        for (int32 i = 0; i < Children->Num(); ++i)
        {
            TSharedRef<SWidget> Child = Children->GetChildAt(i);
            FindAllWidgetsRecursive(Child, OutWidgets);
        }
    }
} 