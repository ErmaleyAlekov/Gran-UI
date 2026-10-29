#include "gallery_events.hpp"

namespace gallery
{
    namespace
    {
        /*
        ** Строка из страницы samples/web_view.xml: «ready» — страница загрузилась, остальное — нажатия.
        ** Ответ уходит обратно в страницу: так видно, что обмен работает в обе стороны.
        **
        ** A string from the samples/web_view.xml page: "ready" — the page has loaded, the rest — the clicks.
        ** The answer goes back into the page: this shows that the exchange works both ways.
        */
        struct PageMessage
        {
            gran::Application* application;
            int* clicks;

            void operator()(gran::ui::Node& webView, const std::string& message) const
            {
                if (message == "ready")
                {
                    setText(*application, "status", "Страница загружена");
                    return;
                }
                ++*clicks;
                setText(*application, "status", "Нажатий: " + std::to_string(*clicks));
                application->postWebMessage(webView, "получено «" + message + "» №" + std::to_string(*clicks));
            }
        };
    }

    // Счётчик живёт до конца процесса: обработчик вызывается, пока работает цикл окна
    // The counter lives until the process ends: the handler is called while the window loop runs
    void registerWeb(gran::Application& application)
    {
        static int clicks = 0;

        application.onWebMessage("fromPage", PageMessage{&application, &clicks});
    }
}
