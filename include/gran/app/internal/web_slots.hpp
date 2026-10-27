#ifndef GRAN_APP_INTERNAL_WEB_SLOTS_HPP
#define GRAN_APP_INTERNAL_WEB_SLOTS_HPP

#include "gran/app/application.hpp"
#include "gran/platform/web_view.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace gran::app
{
    /*
    ** Место одного <WebView> в приложении: узел, браузер (создаётся лениво — при первом показе
    ** узла на экране, поэтому окно без видимых WebView не платит за браузер ни памятью, ни
    ** временем) и то, что браузеру уже передано. Получатель событий браузера — сам слот.
    **
    ** The place of one <WebView> in the application: the node, the browser (created lazily — at the first time
    ** the node is shown on screen, so a window without visible WebViews pays for the browser with neither memory nor
    ** time) and what has already been passed to the browser. The browser event receiver is the slot itself.
    */
    struct WebSlot final : platform::WebViewSink
    {
        // ---- web_slot.cpp ----

        WebSlot(Application::State& owner, ui::Node& webView) noexcept;

        // Строка из страницы — в очередь приложения (обработчик вызовется после событий окна) / A string from the page — into the application queue (the handler is called after the window events)
        void webMessage(const std::string& message) noexcept override;

        // Браузер недоступен: элемент остаётся подложкой, причина — один раз в stderr / The browser is unavailable: the element stays a backdrop, the reason — once into stderr
        void webFailed(long long code) noexcept override;

        Application::State& state;
        ui::Node* node;
        std::unique_ptr<platform::WebView> view;
        // Переданное браузеру: адрес или HTML, границы, видимость / Passed to the browser: the address or HTML, the bounds, the visibility
        std::string source;
        std::string html;
        bool contentSent = false;
        Rect full;
        Rect visible;
        bool shown = false;
        // Сообщения в страницу до создания браузера / Messages into the page before the browser is created
        std::vector<std::string> outbox;
        bool created = false;
        bool failed = false;
        // Номер прохода синхронизации, на котором узел был на экране / The sync pass number at which the node was on screen
        std::uint32_t pass = 0;
    };

    // Строка из страницы, ожидающая обработчика / A string from a page waiting for its handler
    struct WebMessage
    {
        ui::Node* node;
        std::string text;
    };

    // ---- web_views.cpp ----

    // Слот узла (создаётся без браузера, если его нет) / The node slot (created without a browser if there is none)
    WebSlot& webSlot(Application::State& state, ui::Node& node);

    // Снять слоты и сообщения узлов поддерева (браузеры закрываются) / Drop the slots and messages of the subtree nodes (the browsers are closed)
    void forgetWebViews(Application::State& state, const ui::Node& subtree) noexcept;

    // Вызвать обработчики накопленных сообщений; true — были сообщения (нужен кадр) / Call the handlers of the accumulated messages; true — there were messages (a frame is needed)
    bool deliverWebMessages(Application::State& state);

    // ---- web_sync.cpp ----

    // После кадра: создать, расставить, показать или скрыть браузеры по месту узлов на экране / After a frame: create, place, show or hide the browsers by the node places on screen
    void syncWebViews(Application::State& state);

    // ---- web_place.cpp ----

    // Один узел на экране: clip — видимая область предков в логических пикселях окна / One node on screen: clip is the visible area of the ancestors in logical window pixels
    void placeWebView(Application::State& state, ui::Node& node, const Rect& clip, std::uint32_t pass);

    // ---- web_content.cpp ----

    // Создать браузер слота (один раз; nullptr — недоступен) и отдать ему накопленные сообщения / Create the slot browser (once; nullptr — unavailable) and give it the accumulated messages
    void openWebView(Application::State& state, WebSlot& slot);

    // Передать браузеру адрес или HTML узла, если они изменились / Pass the node address or HTML to the browser if they changed
    void sendContent(Application::State& state, WebSlot& slot);

    // ---- web_source.cpp ----

    /*
    ** Адрес страницы из атрибута source: адрес со схемой (https:, file:, about:, data:) — как есть,
    ** иначе путь к файлу относительно каталога разметки → file:///… с экранированием %XX.
    **
    ** The page address from the source attribute: an address with a scheme (https:, file:, about:, data:) — as it is,
    ** otherwise a file path relative to the markup directory → file:///… with %XX escaping.
    */
    std::string webSourceUrl(const std::string& source, const std::string& baseDirectory);
}

#endif
