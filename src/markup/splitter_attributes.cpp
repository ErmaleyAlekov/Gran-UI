#include "gran/markup/internal/build_context.hpp"
#include "gran/markup/live_document.hpp"
#include "gran/style/value_parsers.hpp"

#include "beresta.hpp"

#include <cmath>
#include <sstream>

namespace gran::markup::internal
{
    namespace
    {
        // Положение: число логических пикселей или «NN%» от места под панели / The position: a number of logical pixels or "NN%" of the room for the panes
        void readPosition(ui::SplitterState& split, const std::string& value)
        {
            split.percent = !value.empty() && value.back() == '%';
            if (split.percent)
                split.position = style::parseNumber(value.substr(0, value.size() - 1));
            else
                split.position = style::parseNumber(value);
        }

        // Положение текстом так, как его прочтёт readPosition: «240», «37.5%» (до десятых) / The position as text the way readPosition reads it: "240", "37.5%" (to tenths)
        std::string positionText(const ui::SplitterState& split)
        {
            std::ostringstream text;

            text << std::round(split.position * 10.0f) / 10.0f;
            if (split.percent)
                text << '%';
            return text.str();
        }
    }

    /*
    ** Атрибуты Splitter: position, minFirst, minSecond, collapse (none, first, second), fixed (first, second),
    ** collapsed. Значения уже проверила схема (числа, перечисления); false — атрибут не разделителя.
    **
    ** The Splitter attributes: position, minFirst, minSecond, collapse (none, first, second), fixed (first, second),
    ** collapsed. The schema has already checked the values (numbers, enumerations); false — not a splitter attribute.
    */
    bool readSplitterAttribute(ui::Node& node, std::string_view name, const std::string& value)
    {
        static const style::Keyword COLLAPSES[] = {{"none", 0}, {"first", 1}, {"second", 2}};
        static const style::Keyword FIXES[] = {{"first", 0}, {"second", 1}};
        ui::SplitterState& split = node.content.split;

        if (name == "position")
            readPosition(split, value);
        else if (name == "minFirst")
            split.minFirst = style::parseNumber(value);
        else if (name == "minSecond")
            split.minSecond = style::parseNumber(value);
        else if (name == "collapse")
            split.collapse = static_cast<ui::SplitCollapse>(style::parseKeyword(value, COLLAPSES, 3));
        else if (name == "fixed")
            split.fixed = static_cast<ui::SplitFixed>(style::parseKeyword(value, FIXES, 2));
        else if (name == "collapsed")
            split.collapsed = value == "true" || value == "1";
        else
            return false;
        return true;
    }

}

namespace gran::markup
{
    // Узлы без своего элемента DOM (построены движком) пропускаются; положение «пополам» не пишется / The nodes without their own DOM element (built by the engine) are skipped; the "in half" position is not written
    void writeLiveAttributes(ui::Node& root)
    {
        beresta::Element* element = internal::editableElement(root);
        std::string collapsed = "false";

        if (root.content.split.collapsed)
            collapsed = "true";
        if (root.kind == ui::ElementKind::SPLITTER && element != nullptr)
        {
            if (root.content.split.position >= 0.0f)
                element->setAttribute("position", internal::positionText(root.content.split));
            element->setAttribute("collapsed", collapsed);
        }
        for (const std::unique_ptr<ui::Node>& child : root.children)
            writeLiveAttributes(*child);
    }
}
