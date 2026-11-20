#include "gran/platform/win32/internal/uia_element.hpp"

namespace gran::platform::win32
{
    /*
    ** Таблица «шаблон UI Automation → бит движка». Grid и Table у таблицы идут вместе (бит PATTERN_GRID),
    ** GridItem и TableItem у ячейки — тоже (PATTERN_GRID_ITEM): заголовки столбцов есть у любой таблицы.
    **
    ** The "UI Automation pattern → engine bit" table. Grid and Table of a table go together (the PATTERN_GRID bit),
    ** GridItem and TableItem of a cell too (PATTERN_GRID_ITEM): any table has column headers.
    */
    std::uint16_t patternBit(PATTERNID pattern) noexcept
    {
        static const struct { PATTERNID pattern; std::uint16_t bit; } BITS[] = {
            {UIA_InvokePatternId, a11y::PATTERN_INVOKE}, {UIA_TogglePatternId, a11y::PATTERN_TOGGLE},
            {UIA_SelectionItemPatternId, a11y::PATTERN_SELECT}, {UIA_ValuePatternId, a11y::PATTERN_TEXT},
            {UIA_RangeValuePatternId, a11y::PATTERN_RANGE}, {UIA_ExpandCollapsePatternId, a11y::PATTERN_EXPAND},
            {UIA_GridPatternId, a11y::PATTERN_GRID}, {UIA_TablePatternId, a11y::PATTERN_GRID},
            {UIA_GridItemPatternId, a11y::PATTERN_GRID_ITEM}, {UIA_TableItemPatternId, a11y::PATTERN_GRID_ITEM},
            {UIA_SelectionPatternId, a11y::PATTERN_SELECTION}};

        for (const auto& entry : BITS)
        {
            if (entry.pattern == pattern)
                return entry.bit;
        }
        return 0;
    }

    // Приведение к нужной базе даёт верный указатель интерфейса при множественном наследовании / The cast to the needed base gives the right interface pointer with multiple inheritance
    IUnknown* UiaElement::interfaceOf(PATTERNID pattern) noexcept
    {
        switch (pattern)
        {
            case UIA_InvokePatternId: return static_cast<IInvokeProvider*>(this);
            case UIA_TogglePatternId: return static_cast<IToggleProvider*>(this);
            case UIA_SelectionItemPatternId: return static_cast<ISelectionItemProvider*>(this);
            case UIA_ValuePatternId: return static_cast<IValueProvider*>(this);
            case UIA_RangeValuePatternId: return static_cast<IRangeValueProvider*>(this);
            case UIA_ExpandCollapsePatternId: return static_cast<IExpandCollapseProvider*>(this);
            case UIA_GridPatternId: return static_cast<IGridProvider*>(this);
            case UIA_TablePatternId: return static_cast<ITableProvider*>(this);
            case UIA_GridItemPatternId: return static_cast<IGridItemProvider*>(this);
            case UIA_TableItemPatternId: return static_cast<ITableItemProvider*>(this);
            case UIA_SelectionPatternId: return static_cast<ISelectionProvider*>(this);
            case UIA_VirtualizedItemPatternId: return static_cast<IVirtualizedItemProvider*>(this);
            default: return nullptr;
        }
    }
}
