#include "gran/platform/win32/internal/uia_element.hpp"

#include <utility>

namespace gran::platform::win32
{
    namespace
    {
        // Значения узла, о смене которых сообщают дикторам / The node values whose change is reported to screen readers
        AccessSnapshot snapshotOf(const ui::Node& node)
        {
            a11y::AccessibleInfo info = a11y::describe(node);
            AccessSnapshot snapshot;

            snapshot.node = &node;
            snapshot.checked = info.checked;
            snapshot.current = info.current;
            snapshot.value = std::move(info.value);
            return snapshot;
        }

        // Рассылка изменения свойства и освобождение обоих VARIANT (строки BSTR) / Broadcasting a property change and freeing both VARIANTs (BSTR strings)
        void raiseProperty(UiaElement* element, PROPERTYID property, VARIANT& before, VARIANT& after) noexcept
        {
            // Ошибка рассылки не мешает работе: клиент перечитает свойство сам / A broadcast error does not hinder the work: the client rereads the property itself
            UiaRaiseAutomationPropertyChangedEvent(element, property, before, after);
            VariantClear(&before);
            VariantClear(&after);
        }

        /*
        ** Отметка: у флажка и переключателя — ToggleState, у радиокнопки и вкладки — IsSelected
        ** и событие «элемент выбран» (по нему диктор читает новый выбор).
        **
        ** The mark: for a check box and a switch — ToggleState, for a radio button and a tab — IsSelected
        ** and the "element selected" event (the screen reader reads the new choice by it).
        */
        void raiseChecked(UiaElement* element, std::uint8_t patterns, bool checked) noexcept
        {
            VARIANT before;
            VARIANT after;

            VariantInit(&before);
            VariantInit(&after);
            before.vt = VT_I4;
            after.vt = VT_I4;
            before.lVal = ToggleState_On;
            after.lVal = ToggleState_Off;
            if (checked)
                std::swap(before.lVal, after.lVal);
            if ((patterns & a11y::PATTERN_TOGGLE) != 0)
            {
                raiseProperty(element, UIA_ToggleToggleStatePropertyId, before, after);
                return;
            }
            before.vt = VT_BOOL;
            after.vt = VT_BOOL;
            before.boolVal = VARIANT_TRUE;
            after.boolVal = VARIANT_FALSE;
            if (checked)
                std::swap(before.boolVal, after.boolVal);
            if (checked)
                UiaRaiseAutomationEvent(element, UIA_SelectionItem_ElementSelectedEventId);
            raiseProperty(element, UIA_SelectionItemIsSelectedPropertyId, before, after);
        }

        // Разница прошлого и нового состояния — события тех шаблонов, что есть у узла / The difference of the former and the new state — the events of the patterns the node has
        void raiseDifferences(UiaElement* element, const AccessSnapshot& old, const AccessSnapshot& now)
        {
            std::uint8_t patterns = a11y::patternsOf(*now.node);
            VARIANT before;
            VARIANT after;

            if (old.checked != now.checked && (patterns & (a11y::PATTERN_TOGGLE | a11y::PATTERN_SELECT)) != 0)
                raiseChecked(element, patterns, now.checked);
            if (old.value != now.value && (patterns & a11y::PATTERN_TEXT) != 0)
            {
                VariantInit(&before);
                VariantInit(&after);
                before.vt = VT_BSTR;
                before.bstrVal = SysAllocString(toWide(old.value).c_str());
                after.vt = VT_BSTR;
                after.bstrVal = SysAllocString(toWide(now.value).c_str());
                raiseProperty(element, UIA_ValueValuePropertyId, before, after);
            }
            if (old.current != now.current && (patterns & a11y::PATTERN_RANGE) != 0)
            {
                VariantInit(&before);
                VariantInit(&after);
                before.vt = VT_R8;
                before.dblVal = old.current;
                after.vt = VT_R8;
                after.dblVal = now.current;
                raiseProperty(element, UIA_RangeValueValuePropertyId, before, after);
            }
        }
    }

    /*
    ** Сверка только узла в фокусе: им управляет человек с диктором, и о его изменениях надо сказать
    ** сразу. Новый узел фокуса только запоминается — о нём уже сообщило событие фокуса. Без слушателей
    ** UI Automation сверка не нужна, снимок сбрасывается.
    **
    ** Only the focused node is compared: a person with a screen reader controls it, and its changes must be told
    ** at once. A new focused node is only remembered — the focus event has already reported it. Without UI Automation
    ** listeners the comparison is not needed, the snapshot is reset.
    */
    void raiseStateChanges(const std::shared_ptr<UiaHost>& host) noexcept
    {
        const ui::Node* focus = nullptr;

        if (host->tree != nullptr)
            focus = host->tree->focus;
        try
        {
            if (focus == nullptr || !UiaClientsAreListening())
            {
                host->watched = AccessSnapshot{};
                return;
            }
            AccessSnapshot now = snapshotOf(*focus);
            UiaElement* element = nullptr;
            if (host->watched.node == focus)
                element = elementFor(host, focus);
            if (element != nullptr)
            {
                // Строка не поместилась в память — событие теряется, элемент всё равно освобождается
                // A string did not fit into memory — the event is lost, the element is released anyway
                try
                {
                    raiseDifferences(element, host->watched, now);
                }
                catch (...)
                {
                }
                element->Release();
            }
            host->watched = std::move(now);
        }
        catch (...)
        {
            host->watched = AccessSnapshot{};
        }
    }
}
