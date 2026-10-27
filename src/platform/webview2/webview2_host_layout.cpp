#include "gran/platform/webview2/internal/webview2_host.hpp"

#include <windows.h>

namespace gran::platform
{
    /*
    ** Нет загрузчика WebView2Loader.dll рядом с программой или COM не входит в однопоточный
    ** апартамент — браузер недоступен: получатель узнаёт об этом сразу, вызов возвращает nullptr.
    **
    ** No WebView2Loader.dll loader next to the program or COM cannot enter a single-threaded
    ** apartment — the browser is unavailable: the receiver learns about it at once, the call returns nullptr.
    */
    std::unique_ptr<WebView> createWebView(Window& window, WebViewSink& sink, const WebViewOptions& options)
    {
        webview2::CreateEnvironmentFunction factory = webview2::environmentFactory();

        if (factory == nullptr)
        {
            sink.webFailed(static_cast<long long>(HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND)));
            return nullptr;
        }
        HWND parent = static_cast<HWND>(window.nativeHandle());
        std::shared_ptr<webview2::WebView2Core> core = std::make_shared<webview2::WebView2Core>(parent, sink, options);
        if (!core->start(factory, webview2::browserDataFolder()))
            return nullptr;
        return std::make_unique<webview2::WebView2Host>(core);
    }
}

namespace gran::platform::webview2
{
    void WebView2Host::setBounds(const Rect& full, const Rect& visible)
    {
        _core->setBounds(toRect(full), toRect(visible));
    }

    void WebView2Host::setVisible(bool visible)
    {
        _core->setVisible(visible);
    }
}
