#include "gran/platform/webview2/internal/com_callback.hpp"
#include "gran/platform/webview2/internal/webview2_host.hpp"
#include "gran/platform/win32/win32_window.hpp"

namespace gran::platform::webview2
{
    namespace
    {
        // Обработчик завершения создания контроллера / The controller creation completion handler
        using ControllerHandler = ComCallback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler, WebView2Core,
            HRESULT, ICoreWebView2Controller*>;
    }

    // Среда готова — создаётся контроллер внутри окна-рамки / The environment is ready — a controller is created inside the frame window
    HRESULT WebView2Core::environmentCreated(HRESULT result, ICoreWebView2Environment* environment)
    {
        if (_closed)
            return S_OK;
        if (FAILED(result) || environment == nullptr)
        {
            _sink.webFailed(static_cast<long long>(result));
            return S_OK;
        }
        environment->AddRef();
        *_environment.put() = environment;
        ControllerHandler* handler = new ControllerHandler(weak_from_this(), &WebView2Core::controllerCreated,
            IID_ICoreWebView2CreateCoreWebView2ControllerCompletedHandler);
        HRESULT created = _environment->CreateCoreWebView2Controller(_frame, handler);
        handler->Release();
        if (FAILED(created))
            _sink.webFailed(static_cast<long long>(created));
        return S_OK;
    }

    /*
    ** Контроллер готов: настройки, затем всё, что запросили до готовности (границы, видимость,
    ** содержимое). Если ядро уже закрыто, страница закрывается сразу — иначе процесс браузера
    ** жил бы без окна.
    **
    ** The controller is ready: the settings, then everything requested before readiness (the bounds, the visibility,
    ** the content). If the core is already closed, the page is closed at once — otherwise the browser process
    ** would live without a window.
    */
    HRESULT WebView2Core::controllerCreated(HRESULT result, ICoreWebView2Controller* controller)
    {
        if (_closed && controller != nullptr)
            controller->Close();
        if (_closed)
            return S_OK;
        if (FAILED(result) || controller == nullptr)
        {
            _sink.webFailed(static_cast<long long>(result));
            return S_OK;
        }
        controller->AddRef();
        *_controller.put() = controller;
        HRESULT viewResult = _controller->get_CoreWebView2(_view.put());
        if (FAILED(viewResult) || !_view)
        {
            _sink.webFailed(static_cast<long long>(viewResult));
            return S_OK;
        }
        configure();
        applyBounds();
        applyContent();
        return S_OK;
    }

    // Только строки (не JSON-объекты): разбор формата — дело приложения / Strings only (not JSON objects): parsing the format is the application business
    HRESULT WebView2Core::messageReceived(ICoreWebView2*, ICoreWebView2WebMessageReceivedEventArgs* args)
    {
        LPWSTR text = nullptr;

        if (args != nullptr && SUCCEEDED(args->TryGetWebMessageAsString(&text)) && text != nullptr)
            _sink.webMessage(win32::toUtf8(text));
        CoTaskMemFree(text);
        return S_OK;
    }

    // Страница загружена (или показала ошибку): теперь ей можно слать сообщения / The page is loaded (or showed an error): now messages can be sent to it
    HRESULT WebView2Core::navigationCompleted(ICoreWebView2*, ICoreWebView2NavigationCompletedEventArgs*)
    {
        _loaded = true;
        flushMessages();
        return S_OK;
    }
}
