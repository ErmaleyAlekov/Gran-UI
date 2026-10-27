#ifndef GRAN_STYLE_PROPERTY_HPP
#define GRAN_STYLE_PROPERTY_HPP

#include "gran/style/values.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace gran::style
{
    /*
    ** Все свойства стиля. Порядок совпадает с таблицей PROPERTIES (property_table.cpp);
    ** имя атрибута в разметке — camelCase из этой таблицы.
    **
    ** All style properties. The order matches the PROPERTIES table (property_table.cpp);
    ** the markup attribute name is the camelCase name from that table.
    */
    enum class PropertyId : std::uint8_t
    {
        // ---- Блок / Box ----
        WIDTH, HEIGHT, MIN_WIDTH, MAX_WIDTH, MIN_HEIGHT, MAX_HEIGHT, MARGIN, PADDING,
        GROW, SHRINK, ALIGN_SELF, GRID_ROW, GRID_COLUMN, GRID_ROW_SPAN, GRID_COLUMN_SPAN,
        LEFT, TOP, RIGHT, BOTTOM, Z_INDEX,
        // ---- Контейнер / Container ----
        // OVERFLOW_MODE, а не OVERFLOW: это имя — макрос <math.h> в MSVC / OVERFLOW_MODE, not OVERFLOW: that name is an MSVC <math.h> macro
        ORIENTATION, SPACING, JUSTIFY, ALIGN, WRAP, ROWS, COLUMNS, ROW_SPACING, COLUMN_SPACING, OVERFLOW_MODE,
        // ---- Вид / Visual ----
        BACKGROUND, BORDER_COLOR, BORDER_WIDTH, BORDER_RADIUS, BOX_SHADOW, OPACITY,
        TRANSFORM, TRANSFORM_ORIGIN, BACKDROP_BLUR, CURSOR, SCRIM,
        // ---- Текст (наследуется) / Text (inherited) ----
        COLOR, FONT_FAMILY, FONT_SIZE, FONT_WEIGHT, FONT_STYLE, LINE_HEIGHT, LETTER_SPACING,
        TEXT_ALIGN, TEXT_WRAP, TEXT_OVERFLOW, MAX_LINES, TEXT_DECORATION, ACCENT_COLOR,
        // ---- Движение / Motion ----
        TRANSITION,
        COUNT
    };

    // Число свойств / The property count
    constexpr std::size_t PROPERTY_COUNT = static_cast<std::size_t>(PropertyId::COUNT);

    // Вид значения: какой парсер разбирает текст и какой тип лежит в PropertyValue
    // The value kind: which parser reads the text and which type lies in PropertyValue
    enum class ValueKind : std::uint8_t
    {
        NUMBER, LENGTH, INSETS, RADII, COLOR, PAINT, SHADOWS, TRANSFORM, ORIGIN,
        TRANSITIONS, TRACKS, KEYWORD, INTEGER, BOOLEAN, STRING, FONT_WEIGHT
    };

    // Разобранное значение свойства / A parsed property value
    using PropertyValue = std::variant<float, int, bool, Length, Insets, CornerRadii, Color, Paint,
        std::vector<Shadow>, Transform, Vec2, std::vector<TransitionSpec>, std::vector<GridTrack>, std::string>;

    // Объявленное значение: свойство + значение / A declared value: a property + a value
    using Declaration = std::pair<PropertyId, PropertyValue>;

    // Набор объявлений одного источника (атрибуты элемента, стиль, состояние)
    // A declaration set of one source (element attributes, a style, a state)
    using PropertySet = std::vector<Declaration>;

    // Описание свойства / A property description
    struct PropertyInfo
    {
        const char* name;
        PropertyId id;
        ValueKind kind;
        bool inherited;
    };

    // ---- property_table.cpp ----

    // Описание свойства по идентификатору / The property description by its identifier
    const PropertyInfo& propertyInfo(PropertyId id) noexcept;

    // Поиск по имени атрибута; nullptr — это не свойство стиля
    // A lookup by the attribute name; nullptr — it is not a style property
    const PropertyInfo* findProperty(std::string_view name) noexcept;

    // ---- property_parse.cpp ----

    // Разбор текста атрибута в значение свойства (ошибка — GranException STYLE_VALUE)
    // Parsing an attribute text into a property value (an error is GranException STYLE_VALUE)
    PropertyValue parsePropertyValue(const PropertyInfo& info, std::string_view text);

    // Добавить или заменить объявление (последнее побеждает) / Add or replace a declaration (the last one wins)
    void setDeclaration(PropertySet& set, PropertyId id, PropertyValue value);

    // Объявление свойства в наборе или nullptr / The property declaration in the set or nullptr
    const PropertyValue* findDeclaration(const PropertySet& set, PropertyId id) noexcept;
}

#endif
