#include "gran/platform/win32/internal/uia_element.hpp"

namespace gran::platform::win32
{
    namespace
    {
        /*
        ** ChildrenInvalidated у элемента узла. Идентификатор события — тот же RuntimeId, что отдаёт GetRuntimeId: контейнер
        ** может быть строкой или ячейкой (ушёл редактор ячейки), а у них RuntimeId по записи — с номером узла клиент
        ** не узнал бы элемент (ревью Т-75). Ошибка рассылки не мешает работе: клиент перечитает дерево при следующем обходе.
        **
        ** ChildrenInvalidated at the node element. The event identifier is the same RuntimeId GetRuntimeId gives: the container
        ** may be a row or a cell (the cell editor went away), and theirs is the RuntimeId by the record — with the node serial a client
        ** would not recognize the element (review T-75). A broadcast error does not hinder the work: the client rereads the tree on the next walk.
        */
        void raiseChildrenInvalidated(const std::shared_ptr<UiaHost>& host, const ui::Node* node) noexcept
        {
            UiaElement* element = elementFor(host, node);
            SAFEARRAY* id = nullptr;
            int* parts = nullptr;

            if (element == nullptr)
                return;
            if (SUCCEEDED(element->GetRuntimeId(&id)) && SUCCEEDED(SafeArrayAccessData(id, reinterpret_cast<void**>(&parts))))
            {
                UiaRaiseStructureChangedEvent(element, StructureChangeType_ChildrenInvalidated, parts,
                    static_cast<int>(id->rgsabound[0].cElements));
                SafeArrayUnaccessData(id);
            }
            if (id != nullptr)
                SafeArrayDestroy(id);
            element->Release();
        }
    }

    // NONE в UI Automation не бывает: узел без раскрытия шаблона ExpandCollapse не имеет / There is no NONE in UI Automation: a node without expansion has no ExpandCollapse pattern
    ExpandCollapseState expandCollapseOf(a11y::ExpandState state) noexcept
    {
        switch (state)
        {
            case a11y::ExpandState::LEAF: return ExpandCollapseState_LeafNode;
            case a11y::ExpandState::EXPANDED: return ExpandCollapseState_Expanded;
            default: return ExpandCollapseState_Collapsed;
        }
    }

    /*
    ** Ветку раскрыли или свернули (стрелкой, щелчком, кодом) у строки, на которой стоит диктор: он скажет
    ** «развёрнуто» или «свёрнуто». Ошибка рассылки не мешает работе — клиент перечитает свойство сам.
    **
    ** The branch was expanded or collapsed (by an arrow, a click, the code) at the row the screen reader stands on: it says
    ** "expanded" or "collapsed". A broadcast error does not hinder the work — the client rereads the property itself.
    */
    void raiseExpandChange(UiaElement* element, a11y::ExpandState before, a11y::ExpandState after) noexcept
    {
        VARIANT old;
        VARIANT now;

        if (before == after || before == a11y::ExpandState::NONE || after == a11y::ExpandState::NONE)
            return;
        VariantInit(&old);
        VariantInit(&now);
        old.vt = VT_I4;
        now.vt = VT_I4;
        old.lVal = expandCollapseOf(before);
        now.lVal = expandCollapseOf(after);
        UiaRaiseAutomationPropertyChangedEvent(element, UIA_ExpandCollapseExpandCollapseStatePropertyId, old, now);
    }

    /*
    ** Построенная строка: её дети (ячейки) снова есть. Клиент, прочитавший её, пока она была вне экрана, держит пометку
    ** «без детей» (RuntimeId строки тот же — элемент привязан к записи) — ChildrenInvalidated у самой строки просит перечитать
    ** (найдено ручной проверкой Т-73: Inspect показывал видимые строки без ячеек).
    **
    ** A built row: its children (the cells) exist again. A client that read it while it was offscreen keeps the "no children" mark
    ** (the row RuntimeId is the same — the element is bound to the record) — ChildrenInvalidated at the row itself asks to reread
    ** (found by the manual check T-73: Inspect showed visible rows without cells).
    */
    void raiseRealizedRows(const std::shared_ptr<UiaHost>& host) noexcept
    {
        if (host->tree == nullptr || host->tree->added.empty() || !UiaClientsAreListening())
            return;
        for (const ui::Node* node : host->tree->added)
        {
            if (a11y::contains(*host->tree, node))
                raiseChildrenInvalidated(host, node);
        }
    }

    /*
    ** Строки виртуального списка уходят при прокрутке и перестройке окна строк, на их месте появляются новые узлы.
    ** Клиент (Inspect, диктор) держит элементы старых: без события он показывал бы их до перезапуска и получал бы
    ** UIA_E_ELEMENTNOTAVAILABLE (найдено ручной проверкой Т-73). ChildrenInvalidated у общего контейнера — клиент
    ** перечитывает его детей. Одно событие на кадр, только если клиенты слушают.
    **
    ** The virtual list rows go away on scrolling and on rebuilding the row window, new nodes appear in their place.
    ** A client (Inspect, a screen reader) holds the elements of the old ones: without an event it would show them until a restart and get
    ** UIA_E_ELEMENTNOTAVAILABLE (found by the manual check T-73). ChildrenInvalidated at the common container — the client
    ** rereads its children. One event per frame, only if clients listen.
    */
    void raiseStructureChange(const std::shared_ptr<UiaHost>& host) noexcept
    {
        raiseRealizedRows(host);
        const ui::Node* changed = nullptr;

        if (host->tree != nullptr)
            changed = host->tree->changed;
        if (changed == nullptr || !a11y::contains(*host->tree, changed) || !UiaClientsAreListening())
            return;
        raiseChildrenInvalidated(host, changed);
    }
}
