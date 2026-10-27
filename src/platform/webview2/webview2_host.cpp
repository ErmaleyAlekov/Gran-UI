#include "gran/platform/webview2/internal/webview2_host.hpp"
#include "gran/platform/win32/win32_window.hpp"

#include <utility>

namespace gran::platform::webview2
{
    WebView2Host::WebView2Host(std::shared_ptr<WebView2Core> core) noexcept
        : _core(std::move(core))
    {
    }

    // Колбэки, которые придут позже, найдут ядро уже закрытым или уничтоженным / Callbacks that come later find the core already closed or destroyed
    WebView2Host::~WebView2Host()
    {
        _core->close();
    }

    void WebView2Host::navigate(const std::string& url)
    {
        _core->navigate(win32::toWide(url));
    }

    void WebView2Host::loadHtml(const std::string& html)
    {
        _core->loadHtml(win32::toWide(html));
    }

    void WebView2Host::postMessage(const std::string& message)
    {
        _core->postMessage(win32::toWide(message));
    }
}
