#include "gran/app/internal/app_state.hpp"
#include "gran/core/i18n.hpp"

#include <cstdio>
#include <iostream>

namespace gran::app
{
    namespace
    {
        // Код ОС шестнадцатеричным (HRESULT так и ищут в документации) / The OS code in hexadecimal (an HRESULT is looked up this way)
        std::string hexCode(long long code)
        {
            char text[20] = {};

            std::snprintf(text, sizeof(text), "%08llX", static_cast<unsigned long long>(code) & 0xFFFFFFFFull);
            return text;
        }
    }

    WebSlot::WebSlot(Application::State& owner, ui::Node& webView) noexcept
        : state(owner), node(&webView)
    {
    }

    // Вызывается из колбэка браузера: исключение (нехватка памяти) не выходит наружу — строка теряется
    // Called from a browser callback: an exception (out of memory) does not escape — the string is lost
    void WebSlot::webMessage(const std::string& message) noexcept
    {
        try
        {
            state.webInbox.push_back(WebMessage{node, message});
        }
        catch (...)
        {
            return;
        }
    }

    // Сообщение — один раз на слот: повторные отказы того же браузера не засоряют журнал
    // The message is once per slot: repeated refusals of the same browser do not clutter the log
    void WebSlot::webFailed(long long code) noexcept
    {
        if (failed)
            return;
        failed = true;
        try
        {
            std::cerr << i18n::format(i18n::Message::WEBVIEW_UNAVAILABLE, {hexCode(code)}) << '\n';
        }
        catch (...)
        {
            return;
        }
    }
}
