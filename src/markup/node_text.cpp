#include "gran/markup/internal/build_context.hpp"

#include "beresta.hpp"

namespace gran::markup::internal
{
    namespace
    {
        // Пробелы схлопываются в один, края обрезаются (как white-space: normal в HTML)
        // Spaces collapse into one, the edges are trimmed (as white-space: normal in HTML)
        std::string collapseSpaces(const std::string& text)
        {
            std::string result;
            bool pendingSpace = false;

            for (char symbol : text)
            {
                bool space = symbol == ' ' || symbol == '\t' || symbol == '\n' || symbol == '\r';
                if (space)
                {
                    pendingSpace = !result.empty();
                    continue;
                }
                if (pendingSpace)
                    result += ' ';
                result += symbol;
                pendingSpace = false;
            }
            return result;
        }
    }

    // Переводы строк внутри сохраняются: поле показывает текст так, как он записан
    // The line breaks inside are kept: the field shows the text as it is written
    std::string fieldText(const std::string& text)
    {
        if (!text.empty() && text[0] == '\n')
            return text.substr(1);
        return text;
    }

    /*
    ** Текст внутри тега по виду элемента: Text с Span — участки (дети уже построены, true),
    ** Span — как есть (пробелы схлопнет Text), Text и Button — со схлопнутыми пробелами,
    ** TextArea — с переводами строк. Popup из разметки всегда начинается скрытым.
    **
    ** The text inside the tag by the element kind: a Text with Spans — runs (the children are built already, true),
    ** a Span — as is (the Text collapses the spaces), a Text and a Button — with collapsed spaces,
    ** a TextArea — with line breaks. A Popup from the markup always starts hidden.
    */
    bool readTagText(const beresta::Element& element, ui::Node& node, BuildContext& context)
    {
        ui::ElementKind kind = node.kind;

        if (kind == ui::ElementKind::TEXT && hasSpanElements(element))
        {
            buildRuns(element, node, context);
            return true;
        }
        if (kind == ui::ElementKind::POPUP)
            node.visible = false;
        if (kind == ui::ElementKind::SPAN)
            node.content.text = element.getTextContent();
        if ((kind == ui::ElementKind::TEXT || kind == ui::ElementKind::BUTTON) && node.content.text.empty())
            node.content.text = collapseSpaces(element.getTextContent());
        if (kind == ui::ElementKind::TEXT_AREA && node.content.text.empty())
            node.content.text = fieldText(element.getTextContent());
        return false;
    }
}
