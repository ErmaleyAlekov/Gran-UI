#include "gran/platform/webview2/internal/com_callback.hpp"
#include "gran/platform/webview2/internal/webview2_host.hpp"

namespace gran::platform::webview2
{
    namespace
    {
        // Обработчик завершения создания среды / The environment creation completion handler
        using EnvironmentHandler = ComCallback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler, WebView2Core,
            HRESULT, ICoreWebView2Environment*>;
    }

    WebView2Core::WebView2Core(HWND parent, WebViewSink& sink, const WebViewOptions& options) noexcept
        : _parent(parent), _sink(sink), _options(options)
    {
    }

    /*
    ** Колбэк держит слабую ссылку на ядро (weak_from_this), поэтому start вызывается после
    ** make_shared, а не из конструктора. Своя ссылка на колбэк отпускается сразу: браузер
    ** держит собственную, пока не вызовет его.
    **
    ** The callback holds a weak reference to the core (weak_from_this), so start is called after
    ** make_shared, not from the constructor. Our own reference to the callback is released at once: the browser
    ** keeps its own one until it calls it.
    */
    bool WebView2Core::start(CreateEnvironmentFunction create, const std::wstring& dataFolder)
    {
        const wchar_t* folder = nullptr;

        _frame = createFrameWindow(_parent);
        if (_frame == nullptr)
        {
            _sink.webFailed(static_cast<long long>(HRESULT_FROM_WIN32(GetLastError())));
            return false;
        }
        if (!dataFolder.empty())
            folder = dataFolder.c_str();
        EnvironmentHandler* handler = new EnvironmentHandler(weak_from_this(), &WebView2Core::environmentCreated,
            IID_ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler);
        HRESULT result = create(nullptr, folder, nullptr, handler);
        handler->Release();
        if (SUCCEEDED(result))
            return true;
        _sink.webFailed(static_cast<long long>(result));
        return false;
    }

    /*
    ** Порядок: отписка от событий → Close контроллера (страница и её процесс закрываются) →
    ** ссылки COM → окно-рамка. Результаты remove_* и Close не важны: объект уходит в любом случае.
    **
    ** The order: unsubscribing from the events → the controller Close (the page and its process are closed) →
    ** the COM references → the frame window. The remove_* and Close results do not matter: the object goes away anyway.
    */
    void WebView2Core::close() noexcept
    {
        if (_closed)
            return;
        _closed = true;
        if (_view)
        {
            _view->remove_WebMessageReceived(_messageToken);
            _view->remove_NavigationCompleted(_navigationToken);
        }
        if (_controller)
            _controller->Close();
        _view.reset();
        _controller.reset();
        _environment.reset();
        if (_frame != nullptr)
            DestroyWindow(_frame);
        _frame = nullptr;
    }
}
