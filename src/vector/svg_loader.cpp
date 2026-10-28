#include "gran/vector/internal/vector_internal.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

#include "beresta.hpp"

#include <utility>

namespace gran::vector
{
    namespace
    {
        // Глубина вложенности <g> и разбора (защита от «бомбы» вложенности) / The <g> and parse nesting depth (a guard against a nesting "bomb")
        constexpr int MAX_DEPTH = 64;

        // Значение атрибута по локальному имени; nullptr — нет / The attribute value by the local name; nullptr — none
        const std::string* attributeOf(const beresta::Element& element, std::string_view name) noexcept
        {
            for (const beresta::Attribute& attribute : element.getAttributes())
            {
                if (attribute.name == name)
                    return &attribute.value;
            }
            return nullptr;
        }

        // Число атрибута (единицы вроде «px» после числа не важны); нет — fallback / The attribute number (units like "px" after it do not matter); none — fallback
        float numberOf(const beresta::Element& element, std::string_view name, float fallback)
        {
            const std::string* text = attributeOf(element, name);
            internal::PathScanner scanner{std::string_view(), 0};
            float value = fallback;

            if (text == nullptr)
                return fallback;
            scanner.text = *text;
            if (!internal::readNumber(scanner, value))
                return fallback;
            return value;
        }

        /*
        ** Контур элемента по его виду; незалитый (fill="none") или неизвестный элемент — пустой контур.
        ** У <rect> один заданный радиус скругления служит обоим (как в SVG).
        **
        ** The element contour by its kind; an unfilled (fill="none") or unknown element is an empty contour.
        ** For <rect> one given rounding radius serves both (as in SVG).
        */
        Path elementPath(const beresta::Element& element)
        {
            std::string_view name = element.getLocalName();
            const std::string* fill = attributeOf(element, "fill");
            const std::string* data = attributeOf(element, "d");
            const std::string* points = attributeOf(element, "points");

            if (fill != nullptr && *fill == "none")
                return Path{};
            if (name == "path" && data != nullptr)
                return parsePathData(*data);
            if (name == "rect")
            {
                float rx = numberOf(element, "rx", numberOf(element, "ry", 0.0f));
                Rect rect{numberOf(element, "x", 0.0f), numberOf(element, "y", 0.0f), numberOf(element, "width", 0.0f),
                    numberOf(element, "height", 0.0f)};
                return internal::rectPath(rect, Vec2{rx, numberOf(element, "ry", rx)});
            }
            Vec2 center{numberOf(element, "cx", 0.0f), numberOf(element, "cy", 0.0f)};
            if (name == "circle")
                return internal::ellipsePath(center, Vec2{numberOf(element, "r", 0.0f), numberOf(element, "r", 0.0f)});
            if (name == "ellipse")
                return internal::ellipsePath(center, Vec2{numberOf(element, "rx", 0.0f), numberOf(element, "ry", 0.0f)});
            if ((name == "polygon" || name == "polyline") && points != nullptr)
                return internal::polylinePath(*points, name == "polygon");
            return Path{};
        }

        // Дети-элементы по порядку документа; <g> — внутрь (до предела глубины) / The element children in the document order; <g> — inside (up to the depth limit)
        void walk(const beresta::Element& element, Shape& shape, int depth)
        {
            for (const beresta::Node* node = element.getFirstChild(); node != nullptr; node = node->getNextSibling())
            {
                if (node->getType() != beresta::NodeType::ELEMENT)
                    continue;
                const beresta::Element& child = static_cast<const beresta::Element&>(*node);
                if (child.getLocalName() == "g" && depth < MAX_DEPTH)
                    walk(child, shape, depth + 1);
                Path path = elementPath(child);
                const std::string* rule = attributeOf(child, "fill-rule");
                path.evenOdd = rule != nullptr && *rule == "evenodd";
                if (!path.verbs.empty())
                    shape.paths.push_back(std::move(path));
            }
        }
    }

    /*
    ** Разбор файла (внешние DTD не грузятся — так по умолчанию), корень <svg>, viewBox или width/height
    ** (иначе 24 × 24, как у большинства наборов иконок), обход элементов. Ошибка разбора или пути —
    ** GranException(MARKUP) с именем файла, ошибка чтения — GranException(IO).
    **
    ** Parsing the file (external DTDs are not loaded — that is the default), the <svg> root, viewBox or width/height
    ** (otherwise 24 × 24, as in most icon sets), walking the elements. A parse or path error is
    ** GranException(MARKUP) with the file name, a read error is GranException(IO).
    */
    std::shared_ptr<const Shape> loadSvgFile(const std::string& path)
    {
        beresta::ParserOptions options;
        beresta::Document dom;
        auto shape = std::make_shared<Shape>();

        options.maxDepth = MAX_DEPTH;
        try
        {
            dom = beresta::XmlParser(options).parseFile(path);
        }
        catch (const beresta::ParseException& error)
        {
            throw GranException(ErrorCode::MARKUP, i18n::format(i18n::Message::ICON_BAD_SVG, {path, error.getMessage()}));
        }
        catch (const beresta::DomException&)
        {
            throw GranException(ErrorCode::IO, i18n::format(i18n::Message::IO_READ_FAILED, {path}));
        }
        const beresta::Element* root = dom.getRoot();
        if (root == nullptr || root->getLocalName() != "svg")
            throw GranException(ErrorCode::MARKUP, i18n::format(i18n::Message::ICON_BAD_SVG, {path, "<svg>"}));
        const std::string* viewBox = attributeOf(*root, "viewBox");
        if (viewBox == nullptr || !parseViewBox(*viewBox, shape->viewBox))
            shape->viewBox = Rect{0.0f, 0.0f, numberOf(*root, "width", 24.0f), numberOf(*root, "height", 24.0f)};
        walk(*root, *shape, 0);
        shape->id = nextShapeId();
        return shape;
    }
}
