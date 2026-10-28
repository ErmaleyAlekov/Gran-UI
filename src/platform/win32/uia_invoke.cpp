#include "gran/platform/win32/internal/uia_element.hpp"

namespace gran::platform::win32
{
    /*
    ** Нажатие клиентом (диктор, автотест): то же, что щелчок или Enter, — onClick, открытие меню,
    ** выбор пункта. Выполняется в цикле приложения после возврата: Invoke по правилам UI Automation
    ** не ждёт окончания действия.
    **
    ** A press by a client (a screen reader, a UI test): the same as a click or Enter — onClick, opening a menu,
    ** choosing an item. It is performed in the application loop after the return: by the UI Automation rules Invoke
    ** does not wait for the action to finish.
    */
    HRESULT STDMETHODCALLTYPE UiaElement::Invoke()
    {
        HRESULT status = check(a11y::PATTERN_INVOKE);

        if (FAILED(status))
            return status;
        return post(a11y::AccessRequest{a11y::Request::INVOKE, _node, std::string(), 0.0f});
    }

    // Переключение флажка или переключателя (как пробел): отметка и onChange / Toggling a check box or a switch (as space): the mark and onChange
    HRESULT STDMETHODCALLTYPE UiaElement::Toggle()
    {
        HRESULT status = check(a11y::PATTERN_TOGGLE);

        if (FAILED(status))
            return status;
        return post(a11y::AccessRequest{a11y::Request::TOGGLE, _node, std::string(), 0.0f});
    }

    // Состояний два: неопределённого флажка в движке нет / There are two states: the engine has no indeterminate check box
    HRESULT STDMETHODCALLTYPE UiaElement::get_ToggleState(ToggleState* state)
    {
        HRESULT status = check(a11y::PATTERN_TOGGLE);

        if (state == nullptr)
            return E_POINTER;
        *state = ToggleState_Off;
        if (FAILED(status))
            return status;
        if (_node->content.checked)
            *state = ToggleState_On;
        return S_OK;
    }
}
