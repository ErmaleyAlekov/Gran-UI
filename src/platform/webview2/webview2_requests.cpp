#include "gran/platform/webview2/internal/webview2_host.hpp"
#include "gran/platform/internal/web_message_limits.hpp"
#include "gran/platform/win32/win32_window.hpp"

#include <utility>

namespace gran::platform::webview2
{
    // Адрес заменяет HTML и наоборот: показывается последнее запрошенное / An address replaces HTML and vice versa: the last requested one is shown
    void WebView2Core::navigate(std::wstring url)
    {
        _url = std::move(url);
        _html.clear();
        _contentChanged = true;
        applyContent();
    }

    void WebView2Core::loadHtml(std::wstring html)
    {
        _html = std::move(html);
        _url.clear();
        _contentChanged = true;
        applyContent();
    }

    // До готовности контроллера границы только запоминаются / Before the controller is ready the bounds are only remembered
    void WebView2Core::setBounds(const RECT& full, const RECT& visible)
    {
        _full = full;
        _visible = visible;
        applyBounds();
    }

    void WebView2Core::setVisible(bool visible)
    {
        _shown = visible;
        applyBounds();
    }

    // Сообщения до загрузки страницы ждут: раньше их некому принять / Messages before the page loads wait: nobody can receive them earlier
    void WebView2Core::postMessage(const std::string& message)
    {
        if (_view && _loaded)
        {
            _view->PostWebMessageAsString(win32::toWide(message).c_str());
            return;
        }
        requireWebQueueSpace(_outbox.size(), message.size());
        _outbox.push_back(win32::toWide(message));
    }
}
