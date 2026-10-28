#include "gran/app/internal/app_state.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

#include <utility>

namespace gran
{
    namespace
    {
        // Команды браузеру — только узлам WebView / Browser commands — only for WebView nodes
        void requireWebView(const ui::Node& node)
        {
            if (node.kind != ui::ElementKind::WEB_VIEW)
                throw GranException(ErrorCode::INVALID_ARGUMENT,
                    i18n::format(i18n::Message::WEBVIEW_WRONG_NODE, {ui::elementName(node.kind)}));
        }
    }

    // Обработчик заменяет прежний с тем же именем / The handler replaces a former one with the same name
    void Application::onWebMessage(const std::string& name, WebMessageHandler handler)
    {
        _state->webHandlers[name] = std::move(handler);
    }

    // Браузера ещё нет (узел не показывался) — сообщение ждёт в слоте / No browser yet (the node was not shown) — the message waits in the slot
    void Application::postWebMessage(ui::Node& webView, const std::string& message)
    {
        requireWebView(webView);
        app::WebSlot& slot = app::webSlot(app::windowOf(*_state, webView), webView);
        if (slot.view)
            slot.view->postMessage(message);
        else
            slot.outbox.push_back(message);
    }

    // Новый адрес уйдёт в браузер после ближайшего кадра (sendContent) / The new address goes to the browser after the nearest frame (sendContent)
    void Application::navigate(ui::Node& webView, const std::string& url)
    {
        requireWebView(webView);
        webView.content.source = url;
        ui::markDirty(webView, ui::DIRTY_PAINT);
    }

    void Application::loadHtml(ui::Node& webView, const std::string& html)
    {
        requireWebView(webView);
        webView.content.source.clear();
        webView.content.text = html;
        ui::markDirty(webView, ui::DIRTY_PAINT);
    }
}
