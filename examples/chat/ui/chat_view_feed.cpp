#include "ui/chat_view.hpp"

#include <limits>
#include <utility>

namespace chat
{
    namespace
    {
        // Запас в пикселях: «почти внизу» считается низом (колесо не всегда доводит до конца) / A margin in pixels: "almost at the bottom" counts as the bottom (the wheel does not always reach the end)
        constexpr float FOLLOW_MARGIN = 48.0f;

        /*
        ** Лента у нижнего края — по данным прошлого кадра. Если пользователь прокрутил вверх,
        ** чтобы перечитать, новый текст его не дёргает. Смещение, уже выставленное «в конец»
        ** в этом же пакете задач (ещё без компоновки), тоже считается низом.
        **
        ** The feed is at the bottom edge — by the last frame data. If the user scrolled up
        ** to reread, new text does not jerk them. An offset already set "to the end"
        ** in the same task batch (not laid out yet) counts as the bottom too.
        */
        bool atBottom(const gran::ui::Node& scroll) noexcept
        {
            float hidden = scroll.layout.contentSize.height - scroll.layout.frame.height;

            return scroll.layout.scrollOffset.y >= hidden - FOLLOW_MARGIN;
        }

        // Смещение «в бесконечность»: компоновка ограничит его концом содержимого / The offset "to infinity": the layout clamps it to the content end
        void scrollToEnd(gran::ui::Node& scroll) noexcept
        {
            scroll.layout.scrollOffset.y = std::numeric_limits<float>::max();
            gran::ui::markDirty(scroll, gran::ui::DIRTY_LAYOUT | gran::ui::DIRTY_PAINT);
        }
    }

    /*
    ** Копия шаблона без id корня (иначе find нашёл бы два узла), видимая; Text внутри
    ** ищется по id в копии. Первое сообщение убирает приветствие.
    **
    ** A template copy without the root id (otherwise find would see two nodes), visible; the Text inside
    ** is found by id in the copy. The first message removes the greeting.
    */
    gran::ui::Node& ChatView::addBubble(BubbleRole role, const std::string& text)
    {
        const gran::ui::Node* sample = _userTemplate;
        const char* bodyId = "userBody";

        if (role == BubbleRole::MODEL)
        {
            sample = _modelTemplate;
            bodyId = "modelBody";
        }
        if (_welcome != nullptr)
        {
            _app.remove(*_welcome);
            _welcome = nullptr;
        }
        std::unique_ptr<gran::ui::Node> copy = gran::ui::cloneTree(*sample);
        copy->id.clear();
        copy->visible = true;
        gran::ui::Node& bubble = _app.append(*_feed, std::move(copy));
        gran::ui::Node& body = *gran::ui::findNode(bubble, bodyId);
        if (!text.empty())
            body.content.text = text;
        scrollToEnd(*_scroll);
        return body;
    }

    /*
    ** Ответ разбирается целиком на каждом куске: незакрытый ``` или ** в середине потока
    ** просто остаётся текстом до прихода закрывающего (разбор терпим к обрывам).
    **
    ** The reply is parsed whole on every piece: an unclosed ``` or ** in the middle of the stream
    ** simply stays text until the closing one arrives (the parsing tolerates cuts).
    */
    void ChatView::showReply(gran::ui::Node& body, const std::string& text)
    {
        bool follow = atBottom(*_scroll);

        renderBlocks(body, parseMarkdown(text), "bubbleText");
        if (follow)
            scrollToEnd(*_scroll);
    }

    // Ошибка заменяет заглушку или недописанный ответ; текст ошибки — без разбора Markdown / The error replaces the placeholder or the unfinished reply; the error text is not parsed as Markdown
    void ChatView::showError(gran::ui::Node& body, const std::string& message)
    {
        MarkdownBlock block;

        block.runs.push_back(InlineRun{message, INLINE_PLAIN});
        renderBlocks(body, std::vector<MarkdownBlock>{block}, "errorText");
        scrollToEnd(*_scroll);
    }
}
