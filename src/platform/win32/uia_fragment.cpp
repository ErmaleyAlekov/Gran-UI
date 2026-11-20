#include "gran/platform/win32/internal/uia_element.hpp"

#include <cstdint>

namespace gran::platform::win32
{
    namespace
    {
        // SAFEARRAY из VT_I4; ошибка записи части — массив уничтожается, клиент не получает неполный RuntimeId (ревью Т-75)
        // A SAFEARRAY of VT_I4; a part write error — the array is destroyed, the client gets no partial RuntimeId (review T-75)
        HRESULT runtimeIdArray(int* parts, LONG count, SAFEARRAY** id) noexcept
        {
            *id = SafeArrayCreateVector(VT_I4, 0, static_cast<ULONG>(count));
            if (*id == nullptr)
                return E_OUTOFMEMORY;
            for (LONG index = 0; index < count; ++index)
            {
                HRESULT status = SafeArrayPutElement(*id, &index, &parts[index]);
                if (FAILED(status))
                {
                    SafeArrayDestroy(*id);
                    *id = nullptr;
                    return status;
                }
            }
            return S_OK;
        }
    }

    /*
    ** Обход дерева: направления UI Automation → a11y::navigate. Родителя корня UI Automation
    ** берёт у провайдера HWND, поэтому для корня он пуст.
    **
    ** The tree walk: the UI Automation directions → a11y::navigate. UI Automation takes the root parent
    ** from the HWND provider, so it is empty for the root.
    */
    HRESULT STDMETHODCALLTYPE UiaElement::Navigate(NavigateDirection direction, IRawElementProviderFragment** result)
    {
        static const a11y::Direction DIRECTIONS[] = {a11y::Direction::PARENT, a11y::Direction::NEXT,
            a11y::Direction::PREVIOUS, a11y::Direction::FIRST_CHILD, a11y::Direction::LAST_CHILD};

        Presence state = presence();

        if (result == nullptr)
            return E_POINTER;
        *result = nullptr;
        // Виртуальная строка вне дерева узлов: родитель — таблица или дерево, соседей и детей нет / A virtual row is outside the node tree: the parent is the table or the tree, no siblings and children
        if (state == Presence::VIRTUAL && direction == NavigateDirection_Parent)
            *result = elementFor(_host, a11y::collectionOf(*_key.list));
        if (state == Presence::VIRTUAL)
            return S_OK;
        if (state == Presence::GONE)
            return static_cast<HRESULT>(UIA_E_ELEMENTNOTAVAILABLE);
        if (direction < NavigateDirection_Parent || direction > NavigateDirection_LastChild)
            return E_INVALIDARG;
        const ui::Node* target = a11y::navigate(*_host->tree, *_node, DIRECTIONS[direction]);
        *result = elementFor(_host, target);
        return S_OK;
    }

    /*
    ** UiaAppendRuntimeId + номер узла (две половины). Не адрес: новый узел на месте удалённого получил бы его RuntimeId,
    ** и клиент принял бы новую строку за старую (найдено ручной проверкой Т-73).
    ** Строка списка — по записи: список (две половины номера), запись (две половины: в таблице больше 2^31 строк номера
    ** не совпадут, ревью Т-75), столбец (-1 у строки; столбцов — узлов заголовка — заведомо меньше INT_MAX), версия записей.
    ** Число частей массива не задано заранее: клиент сравнивает массивы целиком.
    **
    ** UiaAppendRuntimeId + the node serial (two halves). Not the address: a new node in place of a removed one would get its RuntimeId,
    ** and a client would take the new row for the old one (found by the manual check T-73).
    ** A list row — by the record: the list (two serial halves), the record (two halves: with more than 2^31 table rows the indexes
    ** do not collide, review T-75), the column (-1 for a row; the columns — header nodes — are surely fewer than INT_MAX), the record version.
    ** The array part count is not fixed: a client compares whole arrays.
    */
    HRESULT STDMETHODCALLTYPE UiaElement::GetRuntimeId(SAFEARRAY** id)
    {
        int parts[7] = {UiaAppendRuntimeId, static_cast<int>(_serial & 0xFFFFFFFFu), static_cast<int>(_serial >> 32), 0, 0, -1, 0};
        LONG count = 3;

        if (id == nullptr)
            return E_POINTER;
        *id = nullptr;
        if (_key.list != nullptr)
        {
            std::uint64_t record = _key.record;
            parts[1] = static_cast<int>(_key.listSerial & 0xFFFFFFFFu);
            parts[2] = static_cast<int>(_key.listSerial >> 32);
            parts[3] = static_cast<int>(record & 0xFFFFFFFFu);
            parts[4] = static_cast<int>(record >> 32);
            if (_key.column != static_cast<std::size_t>(-1))
                parts[5] = static_cast<int>(_key.column & 0x7FFFFFFFu);
            parts[6] = static_cast<int>(_key.version & 0x7FFFFFFFu);
            count = 7;
        }
        return runtimeIdArray(parts, count, id);
    }

    // Рамка узла на экране / The node frame on the screen
    HRESULT STDMETHODCALLTYPE UiaElement::get_BoundingRectangle(UiaRect* rect)
    {
        if (rect == nullptr)
            return E_POINTER;
        *rect = UiaRect{0.0, 0.0, 0.0, 0.0};
        Presence state = presence();
        // Строка вне видимой части — пустая рамка, как у элементов «вне экрана» / A row outside the visible part — an empty frame, as for "offscreen" elements
        if (state == Presence::VIRTUAL)
            return S_OK;
        if (state == Presence::GONE)
            return static_cast<HRESULT>(UIA_E_ELEMENTNOTAVAILABLE);
        *rect = screenRect(*_host, *_node);
        return S_OK;
    }

    /*
    ** Перенос фокуса по просьбе клиента (диктор ведёт фокус за своим курсором): запрос в очередь,
    ** маршрутизатор ставит фокус как Tab. Узел, не принимающий фокус, — UIA_E_INVALIDOPERATION.
    **
    ** Moving the focus at a client request (a screen reader moves the focus after its cursor): a request into the queue,
    ** the router puts the focus as Tab does. A node not taking the focus — UIA_E_INVALIDOPERATION.
    */
    HRESULT STDMETHODCALLTYPE UiaElement::SetFocus()
    {
        Presence state = presence();

        // Строку вне экрана сначала показывают (Realize), фокус — потом / A row outside the screen is shown first (Realize), the focus — after
        if (state == Presence::VIRTUAL)
            return static_cast<HRESULT>(UIA_E_INVALIDOPERATION);
        if (state == Presence::GONE)
            return static_cast<HRESULT>(UIA_E_ELEMENTNOTAVAILABLE);
        if (!a11y::isFocusable(*_node))
            return static_cast<HRESULT>(UIA_E_INVALIDOPERATION);
        return post(a11y::AccessRequest{a11y::Request::FOCUS, _node, std::string(), 0.0f});
    }
}
