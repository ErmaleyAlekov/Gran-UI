#include "gran/style/property.hpp"

#include <cstring>

namespace gran::style
{
    namespace
    {
        using K = ValueKind;
        using P = PropertyId;

        // Все свойства в порядке PropertyId: имя атрибута, вид значения, наследуется ли
        // All properties in the PropertyId order: the attribute name, the value kind, whether inherited
        const PropertyInfo PROPERTIES[PROPERTY_COUNT] = {
            {"width", P::WIDTH, K::LENGTH, false}, {"height", P::HEIGHT, K::LENGTH, false},
            {"minWidth", P::MIN_WIDTH, K::NUMBER, false}, {"maxWidth", P::MAX_WIDTH, K::NUMBER, false},
            {"minHeight", P::MIN_HEIGHT, K::NUMBER, false}, {"maxHeight", P::MAX_HEIGHT, K::NUMBER, false},
            {"margin", P::MARGIN, K::INSETS, false}, {"padding", P::PADDING, K::INSETS, false},
            {"grow", P::GROW, K::NUMBER, false}, {"shrink", P::SHRINK, K::NUMBER, false},
            {"alignSelf", P::ALIGN_SELF, K::KEYWORD, false}, {"gridRow", P::GRID_ROW, K::INTEGER, false},
            {"gridColumn", P::GRID_COLUMN, K::INTEGER, false}, {"gridRowSpan", P::GRID_ROW_SPAN, K::INTEGER, false},
            {"gridColumnSpan", P::GRID_COLUMN_SPAN, K::INTEGER, false}, {"left", P::LEFT, K::NUMBER, false},
            {"top", P::TOP, K::NUMBER, false}, {"right", P::RIGHT, K::NUMBER, false},
            {"bottom", P::BOTTOM, K::NUMBER, false}, {"zIndex", P::Z_INDEX, K::INTEGER, false},
            {"orientation", P::ORIENTATION, K::KEYWORD, false}, {"spacing", P::SPACING, K::NUMBER, false},
            {"justify", P::JUSTIFY, K::KEYWORD, false}, {"align", P::ALIGN, K::KEYWORD, false},
            {"wrap", P::WRAP, K::BOOLEAN, false}, {"rows", P::ROWS, K::TRACKS, false},
            {"columns", P::COLUMNS, K::TRACKS, false}, {"rowSpacing", P::ROW_SPACING, K::NUMBER, false},
            {"columnSpacing", P::COLUMN_SPACING, K::NUMBER, false}, {"overflow", P::OVERFLOW_MODE, K::KEYWORD, false},
            {"background", P::BACKGROUND, K::PAINT, false}, {"borderColor", P::BORDER_COLOR, K::COLOR, false},
            {"borderWidth", P::BORDER_WIDTH, K::INSETS, false}, {"borderRadius", P::BORDER_RADIUS, K::RADII, false},
            {"boxShadow", P::BOX_SHADOW, K::SHADOWS, false}, {"opacity", P::OPACITY, K::NUMBER, false},
            {"transform", P::TRANSFORM, K::TRANSFORM, false}, {"transformOrigin", P::TRANSFORM_ORIGIN, K::ORIGIN, false},
            {"backdropBlur", P::BACKDROP_BLUR, K::NUMBER, false}, {"cursor", P::CURSOR, K::KEYWORD, true},
            {"scrim", P::SCRIM, K::COLOR, false},
            {"color", P::COLOR, K::COLOR, true}, {"fontFamily", P::FONT_FAMILY, K::STRING, true},
            {"fontSize", P::FONT_SIZE, K::NUMBER, true}, {"fontWeight", P::FONT_WEIGHT, K::FONT_WEIGHT, true},
            {"fontStyle", P::FONT_STYLE, K::KEYWORD, true}, {"lineHeight", P::LINE_HEIGHT, K::NUMBER, true},
            {"letterSpacing", P::LETTER_SPACING, K::NUMBER, true}, {"textAlign", P::TEXT_ALIGN, K::KEYWORD, true},
            {"textWrap", P::TEXT_WRAP, K::KEYWORD, true}, {"textOverflow", P::TEXT_OVERFLOW, K::KEYWORD, false},
            {"maxLines", P::MAX_LINES, K::INTEGER, false}, {"textDecoration", P::TEXT_DECORATION, K::KEYWORD, true},
            {"accentColor", P::ACCENT_COLOR, K::COLOR, true},
            {"transition", P::TRANSITION, K::TRANSITIONS, false}, {"animation", P::ANIMATION, K::ANIMATIONS, false}
        };
    }

    // Таблица упорядочена по PropertyId: доступ по индексу
    // The table is ordered by PropertyId: access by index
    const PropertyInfo& propertyInfo(PropertyId id) noexcept
    {
        return PROPERTIES[static_cast<std::size_t>(id)];
    }

    // Линейный поиск по имени: выполняется только при загрузке разметки
    // A linear lookup by name: it runs only while the markup is loaded
    const PropertyInfo* findProperty(std::string_view name) noexcept
    {
        for (const PropertyInfo& info : PROPERTIES)
        {
            if (name.size() == std::strlen(info.name) && name.compare(info.name) == 0)
                return &info;
        }
        return nullptr;
    }
}
