#ifndef CHAT_UI_CHAT_VIEW_HPP
#define CHAT_UI_CHAT_VIEW_HPP

#include "common/markdown.hpp"
#include "gran/app/application.hpp"

#include <cstdint>
#include <string>

namespace chat
{
    // Цвет индикатора состояния (стили dotIdle, dotReady, dotBusy, dotError) / The state indicator color (the dotIdle, dotReady, dotBusy, dotError styles)
    enum class StatusTone : std::uint8_t { IDLE, READY, BUSY, FAILED };

    // Чей пузырь / Whose bubble
    enum class BubbleRole : std::uint8_t { USER, MODEL };

    /*
    ** Представление чата над деревом Gran UI (паттерн Passive View: логики нет, только
    ** «показать»). Узлы ищутся по id разметки chat.xml один раз; новые пузыри — копии
    ** скрытых шаблонов (ui::cloneTree + Application::append). Только поток интерфейса.
    **
    ** The chat view over the Gran UI tree (the Passive View pattern: no logic, only
    ** "show"). The nodes are found by the chat.xml markup ids once; new bubbles are copies
    ** of the hidden templates (ui::cloneTree + Application::append). The UI thread only.
    */
    class ChatView
    {
    public:
        // ---- chat_view.cpp ----

        // Узлы разметки; нет нужного id — ChatException(APP_MISSING_NODE) / The markup nodes; a missing id — ChatException(APP_MISSING_NODE)
        ChatView(gran::Application& application, const std::string& modelName);

        // Текст поля ввода без пробелов по краям; поле очищается / The input text without spaces around; the input is cleared
        std::string takePrompt();

        // Идёт ответ: вместо «Отправить» — «Остановить» / A reply is going on: "Stop" instead of "Send"
        void setBusy(bool busy);

        // Строка состояния и цвет индикатора / The status line and the indicator color
        void setStatus(const std::string& text, StatusTone tone);

        // ---- chat_view_feed.cpp ----

        // Новый пузырь в конце ленты (лента прокручивается к нему); возвращает его тело (Text у вопроса, Stack блоков у ответа)
        // A new bubble at the feed end (the feed scrolls to it); returns its body (a Text for a question, a Stack of blocks for a reply)
        gran::ui::Node& addBubble(BubbleRole role, const std::string& text);

        // Весь текст ответа на сейчас: разбор Markdown и блоки вместо прежних (и вместо заглушки «…»)
        // The whole reply text so far: Markdown parsing and blocks instead of the former ones (and instead of the "…" placeholder)
        void showReply(gran::ui::Node& body, const std::string& text);

        // Текст ошибки в пузыре модели / An error text in the model bubble
        void showError(gran::ui::Node& body, const std::string& message);

    private:
        // ---- chat_view_markdown.cpp ----

        // Блоки в тело ответа вместо прежних детей; textStyle — стиль абзацев / The blocks into the reply body instead of the former children; textStyle is the paragraph style
        void renderBlocks(gran::ui::Node& body, const std::vector<MarkdownBlock>& blocks, const char* textStyle);

        gran::Application& _app;
        gran::ui::Node* _feed;
        gran::ui::Node* _scroll;
        gran::ui::Node* _welcome;
        gran::ui::Node* _userTemplate;
        gran::ui::Node* _modelTemplate;
        gran::ui::Node* _prompt;
        gran::ui::Node* _send;
        gran::ui::Node* _stop;
        gran::ui::Node* _status;
        gran::ui::Node* _dot;
    };
}

#endif
