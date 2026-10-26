#include "ui/chat_view.hpp"

#include <memory>

namespace chat
{
    namespace
    {
        // Новый узел вида kind последним ребёнком parent; style — имя стиля разметки (nullptr — без стиля)
        // A new node of kind kind as the last child of parent; style is a markup style name (nullptr — no style)
        gran::ui::Node& addNode(gran::Application& application, gran::ui::Node& parent, gran::ui::ElementKind kind,
            const char* style)
        {
            std::unique_ptr<gran::ui::Node> node = std::make_unique<gran::ui::Node>();

            node->kind = kind;
            gran::ui::Node& added = application.append(parent, std::move(node));
            if (style != nullptr)
                application.setStyle(added, style);
            return added;
        }

        // Стиль участка по оформлению: код важнее жирного и курсива / The run style by its styling: code wins over bold and italic
        const char* runStyle(std::uint8_t style) noexcept
        {
            if ((style & INLINE_CODE) != 0)
                return "mdCode";
            if ((style & INLINE_BOLD) != 0 && (style & INLINE_ITALIC) != 0)
                return "mdBoldItalic";
            if ((style & INLINE_BOLD) != 0)
                return "mdBold";
            if ((style & INLINE_ITALIC) != 0)
                return "mdItalic";
            return nullptr;
        }

        // Абзац или заголовок — один Text с участками Span / A paragraph or a heading is one Text with Span runs
        void addRuns(gran::Application& application, gran::ui::Node& body, const MarkdownBlock& block, const char* style)
        {
            gran::ui::Node& text = addNode(application, body, gran::ui::ElementKind::TEXT, style);

            for (const InlineRun& run : block.runs)
            {
                gran::ui::Node& span = addNode(application, text, gran::ui::ElementKind::SPAN, runStyle(run.style));
                span.content.text = run.text;
            }
            gran::ui::joinSpans(text);
        }

        // Блок кода: подложка codeBlock, над кодом — язык (если указан), код — моноширинным Text / A code block: the codeBlock backing, the language above the code (if given), the code is a monospace Text
        void addCode(gran::Application& application, gran::ui::Node& body, const MarkdownBlock& block)
        {
            gran::ui::Node& box = addNode(application, body, gran::ui::ElementKind::STACK, "codeBlock");

            if (!block.language.empty())
                addNode(application, box, gran::ui::ElementKind::TEXT, "codeLanguage").content.text = block.language;
            if (!block.runs.empty())
                addNode(application, box, gran::ui::ElementKind::TEXT, "codeText").content.text = block.runs.front().text;
        }
    }

    /*
    ** Содержимое пузыря строится заново из блоков: прежние узлы удаляются через Application::remove
    ** (снимаются ссылки ввода и анимаций), новые — копии разметки не нужны, узлы создаются кодом
    ** со стилями из chat.xml (mdBold, mdCode, codeBlock...). textStyle — стиль абзацев.
    **
    ** The bubble content is built anew from the blocks: the former nodes are removed through Application::remove
    ** (the input and animation references are dropped), the new ones need no markup copies, the nodes are made by code
    ** with the styles from chat.xml (mdBold, mdCode, codeBlock...). textStyle is the paragraph style.
    */
    void ChatView::renderBlocks(gran::ui::Node& body, const std::vector<MarkdownBlock>& blocks, const char* textStyle)
    {
        while (!body.children.empty())
            _app.remove(*body.children.back());
        for (const MarkdownBlock& block : blocks)
        {
            if (block.kind == BlockKind::CODE)
                addCode(_app, body, block);
            else if (block.kind == BlockKind::HEADING)
                addRuns(_app, body, block, "mdHeading");
            else
                addRuns(_app, body, block, textStyle);
        }
    }
}
