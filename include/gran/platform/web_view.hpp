#ifndef GRAN_PLATFORM_WEB_VIEW_HPP
#define GRAN_PLATFORM_WEB_VIEW_HPP

#include "gran/platform/window.hpp"
#include "gran/utils/geometry.hpp"

#include <memory>
#include <string>

namespace gran::platform
{
    /*
    ** Получатель событий встроенного браузера. Вызывается в потоке интерфейса, пока окно
    ** разбирает сообщения ОС (pollEvent), поэтому дерево здесь не правится: события
    ** складываются в очередь и обрабатываются после событий окна. Исключения наружу не выходят
    ** (вызов идёт из колбэка браузера через границу COM).
    **
    ** The embedded browser event receiver. It is called in the UI thread while the window
    ** handles OS messages (pollEvent), so the tree is not edited here: the events
    ** are queued and handled after the window events. No exceptions escape
    ** (the call comes from a browser callback across the COM boundary).
    */
    class WebViewSink
    {
    public:
        virtual ~WebViewSink() = default;

        // Строка из страницы: window.chrome.webview.postMessage("…") / A string from the page: window.chrome.webview.postMessage("…")
        virtual void webMessage(const std::string& message) noexcept = 0;

        // Браузер не создан (нет среды выполнения, загрузчика или доступа к папке данных); code — код ОС
        // The browser was not created (no runtime, loader or access to the data folder); code is the OS code
        virtual void webFailed(long long code) noexcept = 0;
    };

    // Параметры встроенного браузера / The embedded browser parameters
    struct WebViewOptions
    {
        // Инструменты разработчика (F12) — только в отладке / The developer tools (F12) — only for debugging
        bool devTools = false;
    };

    /*
    ** Встроенный браузер системы (паттерн Bridge: интерфейс движка — реализация ОС).
    ** Windows — WebView2 (Chromium Edge), страница живёт в отдельном процессе браузера.
    ** Создание асинхронное: команды до готовности запоминаются и применяются потом
    ** (последний адрес, последние границы, все сообщения по порядку).
    **
    ** The system embedded browser (the Bridge pattern: the engine interface — the OS implementation).
    ** Windows — WebView2 (Chromium Edge), the page lives in a separate browser process.
    ** Creation is asynchronous: commands before readiness are remembered and applied later
    ** (the last address, the last bounds, all messages in order).
    */
    class WebView
    {
    public:
        virtual ~WebView() = default;

        // Перейти по адресу (https://…, file:///…, about:blank) / Navigate to an address (https://…, file:///…, about:blank)
        virtual void navigate(const std::string& url) = 0;

        // Показать готовый HTML (без адреса; ссылки — относительно about:blank) / Show ready HTML (without an address; links are relative to about:blank)
        virtual void loadHtml(const std::string& html) = 0;

        /*
        ** Границы в физических пикселях клиентской области окна: full — весь блок страницы,
        ** visible — его видимая часть (после отсечения прокруткой и overflow предков).
        **
        ** The bounds in physical pixels of the window client area: full is the whole page box,
        ** visible is its visible part (after clipping by the scrolling and overflow of the ancestors).
        */
        virtual void setBounds(const Rect& full, const Rect& visible) = 0;

        // Показать или скрыть страницу (скрытая не рисуется, но живёт) / Show or hide the page (a hidden one is not drawn but stays alive)
        virtual void setVisible(bool visible) = 0;

        // Строка в страницу: событие message у window.chrome.webview (до загрузки — в очередь) / A string into the page: the message event of window.chrome.webview (before loading — queued)
        virtual void postMessage(const std::string& message) = 0;
    };

    // ---- web_view_fallback.cpp / webview2/webview2_host_layout.cpp ----

    // Браузер в окне; nullptr — недоступен на этой ОС или в этой сборке (без SDK WebView2)
    // A browser in the window; nullptr — unavailable on this OS or in this build (without the WebView2 SDK)
    std::unique_ptr<WebView> createWebView(Window& window, WebViewSink& sink, const WebViewOptions& options);
}

#endif
