#include "gran/markup/internal/build_context.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

#include "beresta.hpp"

#include <memory>
#include <utility>

namespace gran::markup::internal
{
    /*
    ** Путь разбирается при загрузке, а не при рисовании: ошибка в данных пути видна сразу, со строкой
    ** и столбцом элемента, как ошибка схемы. Иконка из файла (source) грузится рендерером при показе.
    **
    ** The path is parsed on loading, not on painting: an error in the path data shows at once, with the line
    ** and the column of the element, as a schema error. An icon from a file (source) is loaded by the renderer when shown.
    */
    void readIconShape(const beresta::Element& element, ui::Node& node)
    {
        const std::string* data = findAttribute(element, "data");
        const std::string* viewBox = findAttribute(element, "viewBox");
        const std::string* rule = findAttribute(element, "fillRule");
        auto shape = std::make_shared<vector::Shape>();

        if (data == nullptr)
            return;
        if (viewBox != nullptr && !vector::parseViewBox(*viewBox, shape->viewBox))
            throwAttributeError(element, "viewBox", i18n::format(i18n::Message::ICON_BAD_VIEWBOX, {*viewBox}));
        try
        {
            shape->paths.push_back(vector::parsePathData(*data));
        }
        catch (const GranException& error)
        {
            throwAttributeError(element, "data", error.getMessage());
        }
        shape->paths.front().evenOdd = rule != nullptr && *rule == "evenodd";
        shape->id = vector::nextShapeId();
        node.content.shape = std::move(shape);
    }
}
