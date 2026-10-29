#include "gran/platform/web_view.hpp"

#if !defined(GRAN_HAS_WEBVIEW2)

namespace gran::platform
{
    /*
    ** Сборка без браузера (другая ОС или Windows без SDK WebView2): элемент <WebView> остаётся
    ** обычным блоком со своим фоном, приложение работает дальше. Код 0 говорит получателю, что
    ** браузера нет в самой сборке — так в журнале видно, почему вместо страницы пустой блок.
    **
    ** A build without a browser (another OS or Windows without the WebView2 SDK): the <WebView> element stays
    ** an ordinary box with its background, the application keeps working. The code 0 tells the receiver that
    ** the build itself has no browser — so the log shows why there is an empty box instead of the page.
    */
    std::unique_ptr<WebView> createWebView(Window&, WebViewSink& sink, const WebViewOptions&)
    {
        sink.webFailed(0);
        return nullptr;
    }
}

#endif
