#include "ui/chat_view.hpp"
#include "common/chat_errors.hpp"
#include "common/text_utils.hpp"

namespace chat
{
    namespace
    {
        // Узел по id или исключение: разметку правят отдельно от кода / A node by id or an exception: the markup is edited apart from the code
        gran::ui::Node* need(gran::Application& application, const char* id)
        {
            gran::ui::Node* node = application.find(id);

            if (node == nullptr)
                throw ChatException(ChatText::APP_MISSING_NODE, {id});
            return node;
        }
    }

    // Все узлы ищутся сразу: опечатка в разметке видна при запуске, а не при первом сообщении / All nodes are found at once: a markup typo shows at startup, not at the first message
    ChatView::ChatView(gran::Application& application, const std::string& modelName)
        : _app(application), _feed(need(application, "feed")), _scroll(need(application, "feedScroll")),
          _welcome(need(application, "welcome")), _userTemplate(need(application, "userTemplate")),
          _modelTemplate(need(application, "modelTemplate")), _prompt(need(application, "prompt")),
          _send(need(application, "send")), _stop(need(application, "stop")), _status(need(application, "status")),
          _dot(need(application, "statusDot"))
    {
        gran::ui::Node* name = need(application, "modelName");

        name->content.text = modelName;
        gran::ui::markDirty(*name, gran::ui::DIRTY_LAYOUT | gran::ui::DIRTY_PAINT);
        _app.focus(_prompt);
    }

    // Каретка, выделение и сдвиг длинного текста сбрасываются вместе с текстом / The caret, the selection and the long text shift are reset together with the text
    std::string ChatView::takePrompt()
    {
        std::string text(trimSpaces(_prompt->content.text));

        _prompt->content.text.clear();
        _prompt->content.caret = 0;
        _prompt->content.anchor = 0;
        _prompt->layout.scrollOffset = gran::Vec2{};
        gran::ui::markDirty(*_prompt, gran::ui::DIRTY_LAYOUT | gran::ui::DIRTY_PAINT);
        return text;
    }

    // Видимость меняет компоновку строки ввода; после ответа фокус — снова в поле (можно сразу печатать) / The visibility changes the composer row layout; after a reply the focus is back in the input (one can type at once)
    void ChatView::setBusy(bool busy)
    {
        _send->visible = !busy;
        _stop->visible = busy;
        gran::ui::markDirty(*_send, gran::ui::DIRTY_LAYOUT | gran::ui::DIRTY_PAINT);
        gran::ui::markDirty(*_stop, gran::ui::DIRTY_LAYOUT | gran::ui::DIRTY_PAINT);
        if (!busy)
            _app.focus(_prompt);
    }

    // Цвет индикатора — сменой именованного стиля (переход background описан в разметке) / The indicator color — by changing the named style (the background transition is described in the markup)
    void ChatView::setStatus(const std::string& text, StatusTone tone)
    {
        static const char* const STYLES[] = {"dotIdle", "dotReady", "dotBusy", "dotError"};

        _status->content.text = text;
        gran::ui::markDirty(*_status, gran::ui::DIRTY_LAYOUT | gran::ui::DIRTY_PAINT);
        _app.setStyle(*_dot, STYLES[static_cast<std::size_t>(tone)]);
    }
}
