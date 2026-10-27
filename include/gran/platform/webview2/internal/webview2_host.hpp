#ifndef GRAN_PLATFORM_WEBVIEW2_INTERNAL_WEBVIEW2_HOST_HPP
#define GRAN_PLATFORM_WEBVIEW2_INTERNAL_WEBVIEW2_HOST_HPP

#include "gran/platform/web_view.hpp"
#include "gran/rhi/dx12/internal/com_ref.hpp"

#include <windows.h>

#include "WebView2.h"

#include <memory>
#include <string>
#include <vector>

namespace gran::platform::webview2
{
    // Владеющая ссылка COM — та же обёртка, что у бэкенда D3D12 / An owning COM reference — the same wrapper as the D3D12 backend uses
    template <typename T>
    using ComRef = rhi::dx12::ComRef<T>;

    // Точка входа загрузчика WebView2Loader.dll / The WebView2Loader.dll loader entry point
    using CreateEnvironmentFunction = HRESULT(STDAPICALLTYPE*)(PCWSTR, PCWSTR, ICoreWebView2EnvironmentOptions*,
        ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler*);

    /*
    ** Ядро встроенного браузера: окно-рамка отсечения, среда, контроллер и страница WebView2.
    ** Живёт в shared_ptr: колбэки браузера держат на него слабые ссылки (ComCallback), поэтому
    ** закрытие во время асинхронного создания безопасно. Команды до готовности копятся в полях
    ** и применяются в controllerCreated. Только поток интерфейса (STA).
    **
    ** Окно-рамка (дочернее окно размером с видимую часть) обрезает страницу, частично ушедшую
    ** за край прокрутки: контроллер стоит внутри рамки со сдвигом на невидимую часть.
    **
    ** The embedded browser core: the clipping frame window, the WebView2 environment, controller and page.
    ** It lives in a shared_ptr: the browser callbacks hold weak references to it (ComCallback), so
    ** closing during asynchronous creation is safe. Commands before readiness accumulate in fields
    ** and are applied in controllerCreated. The UI thread (STA) only.
    **
    ** The frame window (a child window the size of the visible part) cuts the page that partly went
    ** past the scrolling edge: the controller sits inside the frame shifted by the invisible part.
    */
    class WebView2Core final : public std::enable_shared_from_this<WebView2Core>
    {
    public:
        // ---- webview2_core.cpp ----

        WebView2Core(HWND parent, WebViewSink& sink, const WebViewOptions& options) noexcept;

        // Окно-рамка и запуск создания среды; false — браузер недоступен (sink.webFailed уже вызван)
        // The frame window and starting the environment creation; false — the browser is unavailable (sink.webFailed is already called)
        bool start(CreateEnvironmentFunction create, const std::wstring& dataFolder);

        // Закрыть страницу и окно-рамку; повторный вызов ничего не делает / Close the page and the frame window; a repeated call does nothing
        void close() noexcept;

        // ---- webview2_requests.cpp ----

        void navigate(std::wstring url);
        void loadHtml(std::wstring html);
        void setBounds(const RECT& full, const RECT& visible);
        void setVisible(bool visible);
        void postMessage(std::wstring message);

        // ---- webview2_callbacks.cpp ----

        HRESULT environmentCreated(HRESULT result, ICoreWebView2Environment* environment);
        HRESULT controllerCreated(HRESULT result, ICoreWebView2Controller* controller);
        HRESULT messageReceived(ICoreWebView2* sender, ICoreWebView2WebMessageReceivedEventArgs* args);
        HRESULT navigationCompleted(ICoreWebView2* sender, ICoreWebView2NavigationCompletedEventArgs* args);

    private:
        // ---- webview2_apply.cpp ----

        // Настройки безопасности страницы и подписка на события / The page security settings and the event subscriptions
        void configure();

        // Применить запомненные границы, видимость, содержимое, сообщения / Apply the remembered bounds, visibility, content, messages
        void applyBounds();
        void applyContent();
        void flushMessages();

        HWND _parent;
        WebViewSink& _sink;
        WebViewOptions _options;
        // Порядок полей = обратный порядок уничтожения: страница → контроллер → среда / The field order = the reverse destruction order: the page → the controller → the environment
        HWND _frame = nullptr;
        ComRef<ICoreWebView2Environment> _environment;
        ComRef<ICoreWebView2Controller> _controller;
        ComRef<ICoreWebView2> _view;
        EventRegistrationToken _messageToken{};
        EventRegistrationToken _navigationToken{};
        RECT _full{};
        RECT _visible{};
        bool _shown = false;
        std::wstring _url;
        std::wstring _html;
        bool _contentChanged = false;
        bool _loaded = false;
        bool _closed = false;
        std::vector<std::wstring> _outbox;
    };

    /*
    ** Реализация platform::WebView: тонкая обёртка над ядром, единственный владелец его
    ** сильной ссылки. Деструктор закрывает страницу сразу, не дожидаясь колбэков.
    **
    ** The platform::WebView implementation: a thin wrapper over the core, the only owner of its
    ** strong reference. The destructor closes the page at once without waiting for callbacks.
    */
    class WebView2Host final : public WebView
    {
    public:
        // ---- webview2_host.cpp ----

        explicit WebView2Host(std::shared_ptr<WebView2Core> core) noexcept;
        ~WebView2Host() override;
        void navigate(const std::string& url) override;
        void loadHtml(const std::string& html) override;
        void postMessage(const std::string& message) override;

        // ---- webview2_host_layout.cpp ----

        void setBounds(const Rect& full, const Rect& visible) override;
        void setVisible(bool visible) override;

    private:
        std::shared_ptr<WebView2Core> _core;
    };

    // ---- webview2_loader.cpp ----

    /*
    ** Точка входа загрузчика: WebView2Loader.dll ищется только рядом с программой и в System32
    ** (не в текущем каталоге — защита от подмены DLL), загружается один раз при первом WebView.
    ** nullptr — загрузчика нет. Заодно — COM в однопоточном апартаменте для потока интерфейса.
    **
    ** The loader entry point: WebView2Loader.dll is searched for only next to the program and in System32
    ** (not in the current directory — a protection against DLL planting), it is loaded once at the first WebView.
    ** nullptr — there is no loader. Also — COM in a single-threaded apartment for the UI thread.
    */
    CreateEnvironmentFunction environmentFactory() noexcept;

    // Папка данных браузера: %LOCALAPPDATA%\GranUI\<имя программы>\WebView2 / The browser data folder: %LOCALAPPDATA%\GranUI\<program name>\WebView2
    std::wstring browserDataFolder();

    // ---- webview2_frame.cpp ----

    // Дочернее окно-рамка отсечения (скрытое) / A child clipping frame window (hidden)
    HWND createFrameWindow(HWND parent) noexcept;

    // Пиксели движка → RECT Win32 (с округлением к целым) / Engine pixels → a Win32 RECT (rounded to integers)
    RECT toRect(const Rect& rect) noexcept;
}

#endif
