#include "gran/platform/win32/internal/uia_element.hpp"
#include "gran/input/input_router.hpp"

namespace gran::platform::win32
{
    /*
    ** Показать строку: запрос в очередь (прокрутку делает цикл приложения), копия появится после компоновки,
    ** и элемент сам к ней привяжется. Уже показанная строка — ничего не делать.
    **
    ** Show the row: a request into the queue (the application loop scrolls), the copy appears after the layout,
    ** and the element binds to it itself. An already shown row — nothing to do.
    */
    HRESULT STDMETHODCALLTYPE UiaElement::Realize()
    {
        Presence state = presence();

        if (state == Presence::ALIVE)
            return S_OK;
        if (state == Presence::GONE)
            return static_cast<HRESULT>(UIA_E_ELEMENTNOTAVAILABLE);
        a11y::AccessRequest request{a11y::Request::REALIZE, _key.list, std::string(), 0.0f};
        request.index = _key.record;
        if (!a11y::postRequest(*_host->tree, request))
            return E_OUTOFMEMORY;
        return S_OK;
    }

    /*
    ** Без узла известны только тип и то, что строки не видно: клиент (Inspect, автотест) видит элемент, а не ошибку,
    ** и может вызвать Realize. Имя и шаблоны появятся, когда копия будет построена.
    **
    ** Without a node only the type and that the row is not shown are known: a client (Inspect, a UI test) sees an element, not an error,
    ** and may call Realize. The name and the patterns appear when the copy is built.
    */
    bool UiaElement::virtualProperty(PROPERTYID property, VARIANT* value) const noexcept
    {
        value->vt = VT_I4;
        switch (property)
        {
            case UIA_ControlTypePropertyId:
                value->lVal = UIA_DataItemControlTypeId;
                if (_key.role == a11y::Role::TREE_ITEM)
                    value->lVal = UIA_TreeItemControlTypeId;
                return true;
            case UIA_IsOffscreenPropertyId: case UIA_IsControlElementPropertyId: case UIA_IsContentElementPropertyId:
                value->vt = VT_BOOL;
                value->boolVal = VARIANT_TRUE;
                return true;
            // Как у построенной строки: отключён сам список или предок — строка отключена (ревью Т-75) / As for a built row: the list itself or an ancestor is disabled — the row is disabled (review T-75)
            case UIA_IsEnabledPropertyId:
                value->vt = VT_BOOL;
                value->boolVal = VARIANT_TRUE;
                if (input::disabled(_key.list))
                    value->boolVal = VARIANT_FALSE;
                return true;
            default:
                value->vt = VT_EMPTY;
                return false;
        }
    }
}
