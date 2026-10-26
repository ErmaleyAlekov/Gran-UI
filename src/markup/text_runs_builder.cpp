#include "gran/markup/internal/build_context.hpp"

#include "beresta.hpp"

#include <string>
#include <vector>

namespace gran::markup::internal
{
    namespace
    {
        // Пробельный символ XML / An XML whitespace character
        bool isSpace(char symbol) noexcept
        {
            return symbol == ' ' || symbol == '\t' || symbol == '\n' || symbol == '\r';
        }

        // Анонимный участок: текст между Span наследует всё от Text / An anonymous run: the text between Spans inherits everything from Text
        void appendPlain(ui::Node& text, const std::string& value)
        {
            std::unique_ptr<ui::Node> span = std::make_unique<ui::Node>();

            span->kind = ui::ElementKind::SPAN;
            span->content.text = value;
            ui::appendChild(text, std::move(span));
        }

        /*
        ** Пробелы схлопываются через границы участков (как white-space: normal в HTML), края всего
        ** текста обрезаются. Схлопнутый пробел остаётся в том участке, где начался промежуток:
        ** у «слово <Span>код</Span>» пробел — в обычном участке, а не на подложке кода.
        **
        ** Spaces collapse across the run boundaries (as white-space: normal in HTML), the edges of the whole
        ** text are trimmed. A collapsed space stays in the run where the gap began:
        ** in "word <Span>code</Span>" the space is in the plain run, not on the code background.
        */
        void collapseRuns(ui::Node& text)
        {
            std::vector<std::string> parts(text.children.size());
            std::size_t owner = 0;
            bool pending = false;
            bool started = false;

            for (std::size_t index = 0; index < parts.size(); ++index)
            {
                for (char symbol : text.children[index]->content.text)
                {
                    // Первый пробел промежутка запоминает свой участок; остальные пробелы пропускаются
                    // The first gap space remembers its run; the other spaces are skipped
                    if (isSpace(symbol) && started && !pending)
                    {
                        pending = true;
                        owner = index;
                    }
                    if (isSpace(symbol))
                        continue;
                    if (pending)
                        parts[owner] += ' ';
                    parts[index] += symbol;
                    started = true;
                    pending = false;
                }
            }
            for (std::size_t index = 0; index < parts.size(); ++index)
                text.children[index]->content.text = parts[index];
        }
    }

    // Есть ли среди детей элемент Span / Whether there is a Span element among the children
    bool hasSpanElements(const beresta::Element& element) noexcept
    {
        for (const beresta::Element* child = firstElementChild(element); child != nullptr;
             child = nextElementSibling(*child))
        {
            if (child->getLocalName() == "Span")
                return true;
        }
        return false;
    }

    /*
    ** Смешанное содержимое Text по порядку: текстовые узлы — анонимные участки, элементы Span —
    ** узлы со своими атрибутами и стилем. Текст узла Text — склейка участков (по нему работают
    ** выделение, копирование и доступность).
    **
    ** The Text mixed content in order: text nodes are anonymous runs, Span elements are
    ** nodes with their own attributes and style. The Text node text is the runs joined (selection,
    ** copying and accessibility work on it).
    */
    void buildRuns(const beresta::Element& element, ui::Node& text, BuildContext& context)
    {
        for (const beresta::Node* child = element.getFirstChild(); child != nullptr; child = child->getNextSibling())
        {
            beresta::NodeType type = child->getType();
            if (type == beresta::NodeType::TEXT || type == beresta::NodeType::CDATA_SECTION)
                appendPlain(text, static_cast<const beresta::CharacterNode*>(child)->getValue());
            else if (type == beresta::NodeType::ELEMENT
                && static_cast<const beresta::Element*>(child)->getLocalName() == "Span")
                ui::appendChild(text, buildNode(*static_cast<const beresta::Element*>(child), context));
        }
        collapseRuns(text);
        ui::joinSpans(text);
    }
}
