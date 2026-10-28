#include "gran/platform/win32/internal/uia_element.hpp"

namespace gran::platform::win32
{
    /*
    ** Замена всего текста поля (как «выделить всё и вставить»): переводы строк, maxLength и onChange —
    ** как у вставки. Поле только для чтения и ComboBox (значение — выбор пункта) отказывают сразу.
    **
    ** Replacing the whole input text (as "select all and paste"): the line breaks, maxLength and onChange are
    ** as for pasting. A read-only input and a ComboBox (the value is an item choice) refuse at once.
    */
    HRESULT STDMETHODCALLTYPE UiaElement::SetValue(LPCWSTR text)
    {
        HRESULT status = check(a11y::PATTERN_TEXT);

        if (FAILED(status))
            return status;
        if (text == nullptr)
            return E_INVALIDARG;
        if (a11y::isReadOnly(*_node))
            return static_cast<HRESULT>(UIA_E_ELEMENTNOTENABLED);
        try
        {
            return post(a11y::AccessRequest{a11y::Request::SET_TEXT, _node, toUtf8(text), 0.0f});
        }
        catch (...)
        {
            return E_OUTOFMEMORY;
        }
    }

    // Текст поля или подпись выбранного пункта; пароль не раскрывается (пустая строка) / The input text or the selected item label; a password is not disclosed (an empty string)
    HRESULT STDMETHODCALLTYPE UiaElement::get_Value(BSTR* text)
    {
        HRESULT status = check(a11y::PATTERN_TEXT);

        if (text == nullptr)
            return E_POINTER;
        *text = nullptr;
        if (FAILED(status))
            return status;
        try
        {
            *text = SysAllocString(toWide(a11y::describe(*_node).value).c_str());
        }
        catch (...)
        {
            return E_OUTOFMEMORY;
        }
        if (*text == nullptr)
            return E_OUTOFMEMORY;
        return S_OK;
    }

    // Общий у Value и RangeValue (одинаковая сигнатура): ответ по самому узлу / Shared by Value and RangeValue (the same signature): the answer is by the node itself
    HRESULT STDMETHODCALLTYPE UiaElement::get_IsReadOnly(BOOL* readOnly)
    {
        if (readOnly == nullptr)
            return E_POINTER;
        *readOnly = TRUE;
        if (!alive())
            return static_cast<HRESULT>(UIA_E_ELEMENTNOTAVAILABLE);
        if (!a11y::isReadOnly(*_node))
            *readOnly = FALSE;
        return S_OK;
    }

    // Большой шаг диапазона — десять малых (PageUp/PageDown у системного ползунка) / The large range step is ten small ones (PageUp/PageDown of the system slider)
    HRESULT STDMETHODCALLTYPE UiaElement::get_LargeChange(double* step)
    {
        return rangeNumber(step, &a11y::AccessibleInfo::largeStep);
    }
}
