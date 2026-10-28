#include "gran/app/internal/app_state.hpp"

namespace gran::app
{
    // Сообщения, отправленные до создания браузера, уходят в него сразу (он сам ждёт загрузки страницы)
    // Messages sent before the browser was created go into it at once (it waits for the page load itself)
    void openWebView(AppWindow& win, WebSlot& slot)
    {
        platform::WebViewOptions options;

        slot.created = true;
        options.devTools = win.shared.options.debugLayer;
        slot.view = platform::createWebView(*win.window, slot, options);
        if (!slot.view)
            return;
        for (const std::string& message : slot.outbox)
            slot.view->postMessage(message);
        slot.outbox.clear();
    }

    /*
    ** Адрес важнее HTML: при source в браузер уходит адрес, иначе — текст тега как HTML.
    ** Передаётся только изменившееся (Application::navigate и loadHtml меняют узел).
    **
    ** The address wins over HTML: with source the browser gets the address, otherwise the tag text as HTML.
    ** Only the changed one is passed (Application::navigate and loadHtml change the node).
    */
    void sendContent(AppWindow& win, WebSlot& slot)
    {
        const ui::NodeContent& content = slot.node->content;

        if (!content.source.empty())
        {
            if (slot.contentSent && content.source == slot.source)
                return;
            slot.view->navigate(webSourceUrl(content.source, win.document.baseDirectory));
        }
        else
        {
            if (slot.contentSent && slot.source.empty() && content.text == slot.html)
                return;
            slot.view->loadHtml(content.text);
        }
        slot.source = content.source;
        slot.html = content.text;
        slot.contentSent = true;
    }
}
