#include "gran/platform/webview2/internal/com_callback.hpp"
#include "gran/platform/webview2/internal/webview2_host.hpp"

namespace gran::platform::webview2
{
    namespace
    {
        using MessageHandler = ComCallback<ICoreWebView2WebMessageReceivedEventHandler, WebView2Core,
            ICoreWebView2*, ICoreWebView2WebMessageReceivedEventArgs*>;
        using NavigationHandler = ComCallback<ICoreWebView2NavigationCompletedEventHandler, WebView2Core,
            ICoreWebView2*, ICoreWebView2NavigationCompletedEventArgs*>;
    }

    /*
    ** Мост — только строковые сообщения: объекты программы в скрипт страницы не попадают
    ** (AreHostObjectsAllowed = FALSE), строка состояния выключена, инструменты разработчика —
    ** только по WebViewOptions::devTools. Отказ отдельной настройки не мешает работе страницы,
    ** поэтому результаты put_* не проверяются; отказ подписки на события — ошибка браузера.
    **
    ** The bridge is string messages only: the program objects do not get into the page script
    ** (AreHostObjectsAllowed = FALSE), the status bar is off, the developer tools are
    ** only by WebViewOptions::devTools. A refused single setting does not stop the page from working,
    ** so the put_* results are not checked; a refused event subscription is a browser error.
    */
    void WebView2Core::configure()
    {
        ComRef<ICoreWebView2Settings> settings;

        if (SUCCEEDED(_view->get_Settings(settings.put())) && settings)
        {
            settings->put_AreHostObjectsAllowed(FALSE);
            settings->put_IsStatusBarEnabled(FALSE);
            settings->put_IsWebMessageEnabled(TRUE);
            settings->put_AreDevToolsEnabled(_options.devTools);
        }
        MessageHandler* message = new MessageHandler(weak_from_this(), &WebView2Core::messageReceived,
            IID_ICoreWebView2WebMessageReceivedEventHandler);
        NavigationHandler* navigation = new NavigationHandler(weak_from_this(), &WebView2Core::navigationCompleted,
            IID_ICoreWebView2NavigationCompletedEventHandler);
        HRESULT messageResult = _view->add_WebMessageReceived(message, &_messageToken);
        HRESULT navigationResult = _view->add_NavigationCompleted(navigation, &_navigationToken);
        message->Release();
        navigation->Release();
        if (FAILED(messageResult))
            _sink.webFailed(static_cast<long long>(messageResult));
        else if (FAILED(navigationResult))
            _sink.webFailed(static_cast<long long>(navigationResult));
    }

    /*
    ** Рамка — видимая часть, контроллер внутри неё сдвинут на невидимую: страница, частично
    ** ушедшая за край прокрутки, обрезается краем рамки, а не сжимается. Пустая видимая часть
    ** или setVisible(false) — рамка и страница скрыты (страница не рисуется, но живёт).
    **
    ** The frame is the visible part, the controller inside it is shifted by the invisible one: a page partly
    ** gone past the scrolling edge is cut by the frame edge instead of being squeezed. An empty visible part
    ** or setVisible(false) — the frame and the page are hidden (the page is not drawn but stays alive).
    */
    void WebView2Core::applyBounds()
    {
        bool shown = _shown && _visible.right > _visible.left && _visible.bottom > _visible.top;
        RECT inner;

        if (!_controller || _frame == nullptr)
            return;
        SetWindowPos(_frame, nullptr, _visible.left, _visible.top, _visible.right - _visible.left,
            _visible.bottom - _visible.top, SWP_NOZORDER | SWP_NOACTIVATE);
        inner.left = _full.left - _visible.left;
        inner.top = _full.top - _visible.top;
        inner.right = inner.left + (_full.right - _full.left);
        inner.bottom = inner.top + (_full.bottom - _full.top);
        _controller->put_Bounds(inner);
        if (shown)
            ShowWindow(_frame, SW_SHOWNA);
        else
            ShowWindow(_frame, SW_HIDE);
        _controller->put_IsVisible(shown);
    }

    // Неверный адрес оставляет прежнюю страницу: результат Navigate сознательно не проверяется
    // An invalid address keeps the former page: the Navigate result is deliberately not checked
    void WebView2Core::applyContent()
    {
        if (!_view || !_contentChanged)
            return;
        _contentChanged = false;
        _loaded = false;
        if (!_url.empty())
            _view->Navigate(_url.c_str());
        else
            _view->NavigateToString(_html.c_str());
    }

    // Страница без обработчика message просто не услышит строку — результат не важен
    // A page without a message handler simply does not hear the string — the result does not matter
    void WebView2Core::flushMessages()
    {
        if (!_view || !_loaded)
            return;
        for (const std::wstring& message : _outbox)
            _view->PostWebMessageAsString(message.c_str());
        _outbox.clear();
    }
}
