#include "gran/platform/web_view.hpp"

#if !defined(GRAN_HAS_WEBVIEW2)

namespace gran::platform
{
    /*
    ** Сборка без браузера (другая ОС или Windows без SDK WebView2): элемент <WebView> остаётся
    ** обычным блоком со своим фоном, приложение работает дальше.
    **
    ** A build without a browser (another OS or Windows without the WebView2 SDK): the <WebView> element stays
    ** an ordinary box with its background, the application keeps working.
    */
    std::unique_ptr<WebView> createWebView(Window&, WebViewSink&, const WebViewOptions&)
    {
        return nullptr;
    }
}

#endif
