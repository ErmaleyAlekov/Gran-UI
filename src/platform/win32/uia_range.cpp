#include "gran/platform/win32/internal/uia_element.hpp"

#include <cmath>

namespace gran::platform::win32
{
    /*
    ** Число ползунка: вне [минимум, максимум] — E_INVALIDARG (так требует RangeValue), внутри — запрос;
    ** округление к шагу и onChange — как у перетаскивания. Полоса прогресса меняется только кодом.
    **
    ** The slider number: outside [minimum, maximum] — E_INVALIDARG (as RangeValue requires), inside — a request;
    ** rounding to the step and onChange are as for dragging. A progress bar changes only by the code.
    */
    HRESULT STDMETHODCALLTYPE UiaElement::SetValue(double value)
    {
        HRESULT status = check(a11y::PATTERN_RANGE);

        if (FAILED(status))
            return status;
        if (a11y::isReadOnly(*_node))
            return static_cast<HRESULT>(UIA_E_ELEMENTNOTENABLED);
        if (std::isnan(value) || value < _node->content.minimum || value > _node->content.maximum)
            return E_INVALIDARG;
        return post(a11y::AccessRequest{a11y::Request::SET_VALUE, _node, std::string(), static_cast<float>(value)});
    }

    // Текущее число / The current number
    HRESULT STDMETHODCALLTYPE UiaElement::get_Value(double* value)
    {
        return rangeNumber(value, &a11y::AccessibleInfo::current);
    }

    // Верх диапазона / The range top
    HRESULT STDMETHODCALLTYPE UiaElement::get_Maximum(double* maximum)
    {
        return rangeNumber(maximum, &a11y::AccessibleInfo::maximum);
    }

    // Низ диапазона / The range bottom
    HRESULT STDMETHODCALLTYPE UiaElement::get_Minimum(double* minimum)
    {
        return rangeNumber(minimum, &a11y::AccessibleInfo::minimum);
    }

    // Малый шаг — как у стрелок ползунка / The small step is as for the slider arrows
    HRESULT STDMETHODCALLTYPE UiaElement::get_SmallChange(double* step)
    {
        return rangeNumber(step, &a11y::AccessibleInfo::smallStep);
    }
}
