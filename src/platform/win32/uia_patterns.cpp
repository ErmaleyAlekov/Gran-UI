#include "gran/platform/win32/internal/uia_element.hpp"
#include "gran/input/input_router.hpp"

namespace gran::platform::win32
{
    /*
    ** Шаблон отдаётся, только если узел жив и поддерживает его: клиент по ответу решает, что показать
    ** (UI Automation сама выводит из этого свойства Is*PatternAvailable).
    **
    ** A pattern is given only if the node is alive and supports it: the client decides what to show by the answer
    ** (UI Automation itself derives the Is*PatternAvailable properties from it).
    */
    IUnknown* UiaElement::patternFor(PATTERNID pattern) noexcept
    {
        std::uint8_t patterns = 0;

        if (alive())
            patterns = a11y::patternsOf(*_node);
        if (pattern == UIA_InvokePatternId && (patterns & a11y::PATTERN_INVOKE) != 0)
            return static_cast<IInvokeProvider*>(this);
        if (pattern == UIA_TogglePatternId && (patterns & a11y::PATTERN_TOGGLE) != 0)
            return static_cast<IToggleProvider*>(this);
        if (pattern == UIA_SelectionItemPatternId && (patterns & a11y::PATTERN_SELECT) != 0)
            return static_cast<ISelectionItemProvider*>(this);
        if (pattern == UIA_ValuePatternId && (patterns & a11y::PATTERN_TEXT) != 0)
            return static_cast<IValueProvider*>(this);
        if (pattern == UIA_RangeValuePatternId && (patterns & a11y::PATTERN_RANGE) != 0)
            return static_cast<IRangeValueProvider*>(this);
        return nullptr;
    }

    // QueryInterface по IID шаблона — тот же ответ, что у GetPatternProvider / QueryInterface by the pattern IID gives the same answer as GetPatternProvider
    IUnknown* UiaElement::patternByIid(REFIID riid) noexcept
    {
        static const struct { const IID* iid; PATTERNID pattern; } PATTERNS[] = {
            {&__uuidof(IInvokeProvider), UIA_InvokePatternId}, {&__uuidof(IToggleProvider), UIA_TogglePatternId},
            {&__uuidof(ISelectionItemProvider), UIA_SelectionItemPatternId},
            {&__uuidof(IValueProvider), UIA_ValuePatternId}, {&__uuidof(IRangeValueProvider), UIA_RangeValuePatternId}};

        for (const auto& entry : PATTERNS)
        {
            if (riid == *entry.iid)
                return patternFor(entry.pattern);
        }
        return nullptr;
    }

    // Клиент мог держать шаблон дольше узла или после смены вида узла / A client may hold a pattern longer than the node or after the node kind changed
    HRESULT UiaElement::check(std::uint8_t pattern) const noexcept
    {
        if (!alive())
            return static_cast<HRESULT>(UIA_E_ELEMENTNOTAVAILABLE);
        if ((a11y::patternsOf(*_node) & pattern) == 0)
            return static_cast<HRESULT>(UIA_E_INVALIDOPERATION);
        return S_OK;
    }

    /*
    ** Отключённый узел (сам или через предка) отвечает ошибкой сразу: клиент узнаёт об отказе
    ** в том же вызове. Остальные проверки (модальный диалог, maxLength) — при выполнении запроса.
    **
    ** A disabled node (itself or through an ancestor) answers with an error at once: the client learns of the refusal
    ** in the same call. The other checks (a modal dialog, maxLength) happen when the request is performed.
    */
    HRESULT UiaElement::post(const a11y::AccessRequest& request) noexcept
    {
        if (input::disabled(_node))
            return static_cast<HRESULT>(UIA_E_ELEMENTNOTENABLED);
        if (!a11y::postRequest(*_host->tree, request))
            return E_OUTOFMEMORY;
        return S_OK;
    }

    // describe собирает строки (имя) и может бросить — исключение не уходит в UI Automation / describe builds strings (the name) and may throw — the exception does not leave into UI Automation
    HRESULT UiaElement::rangeNumber(double* result, float a11y::AccessibleInfo::* field) const noexcept
    {
        HRESULT status = check(a11y::PATTERN_RANGE);

        if (result == nullptr)
            return E_POINTER;
        *result = 0.0;
        if (FAILED(status))
            return status;
        try
        {
            *result = static_cast<double>(a11y::describe(*_node).*field);
        }
        catch (...)
        {
            return E_OUTOFMEMORY;
        }
        return S_OK;
    }
}
