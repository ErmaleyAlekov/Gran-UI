#include "gran/style/style_sheet.hpp"

namespace gran::style
{
    namespace
    {
        using S = ComputedStyle;
        using P = PropertyId;

        /*
        ** Поле вычисленного стиля по свойству: указатель на член структуры. Таблицы
        ** по типу значения заменяют огромный switch: добавить свойство — одна строка.
        **
        ** A computed style field by property: a pointer to a structure member. Tables
        ** by value type replace a huge switch: adding a property is one line.
        */
        template <typename T>
        struct Field
        {
            PropertyId id;
            T ComputedStyle::* member;
        };

        const Field<float> FLOATS[] = {
            {P::MIN_WIDTH, &S::minWidth}, {P::MAX_WIDTH, &S::maxWidth}, {P::MIN_HEIGHT, &S::minHeight},
            {P::MAX_HEIGHT, &S::maxHeight}, {P::GROW, &S::grow}, {P::SHRINK, &S::shrink}, {P::LEFT, &S::left},
            {P::TOP, &S::top}, {P::RIGHT, &S::right}, {P::BOTTOM, &S::bottom}, {P::SPACING, &S::spacing},
            {P::ROW_SPACING, &S::rowSpacing}, {P::COLUMN_SPACING, &S::columnSpacing}, {P::OPACITY, &S::opacity},
            {P::BACKDROP_BLUR, &S::backdropBlur}, {P::FONT_SIZE, &S::fontSize}, {P::LINE_HEIGHT, &S::lineHeight},
            {P::LETTER_SPACING, &S::letterSpacing}};
        const Field<int> INTS[] = {
            {P::ALIGN_SELF, &S::alignSelf}, {P::GRID_ROW, &S::gridRow}, {P::GRID_COLUMN, &S::gridColumn},
            {P::GRID_ROW_SPAN, &S::gridRowSpan}, {P::GRID_COLUMN_SPAN, &S::gridColumnSpan}, {P::Z_INDEX, &S::zIndex},
            {P::ORIENTATION, &S::orientation}, {P::JUSTIFY, &S::justify}, {P::ALIGN, &S::align},
            {P::OVERFLOW_MODE, &S::overflow}, {P::CURSOR, &S::cursor}, {P::FONT_WEIGHT, &S::fontWeight},
            {P::FONT_STYLE, &S::fontStyle}, {P::TEXT_ALIGN, &S::textAlign}, {P::TEXT_WRAP, &S::textWrap},
            {P::TEXT_OVERFLOW, &S::textOverflow}, {P::MAX_LINES, &S::maxLines},
            {P::TEXT_DECORATION, &S::textDecoration}};
        const Field<Length> LENGTHS[] = {{P::WIDTH, &S::width}, {P::HEIGHT, &S::height}};
        const Field<Insets> INSETS[] = {{P::MARGIN, &S::margin}, {P::PADDING, &S::padding},
            {P::BORDER_WIDTH, &S::borderWidth}};
        const Field<bool> BOOLS[] = {{P::WRAP, &S::wrap}};
        const Field<std::vector<GridTrack>> TRACKS[] = {{P::ROWS, &S::rows}, {P::COLUMNS, &S::columns}};
        const Field<Paint> PAINTS[] = {{P::BACKGROUND, &S::background}};
        const Field<Color> COLORS[] = {{P::BORDER_COLOR, &S::borderColor}, {P::COLOR, &S::color},
            {P::ACCENT_COLOR, &S::accentColor}, {P::SCRIM, &S::scrim}};
        const Field<CornerRadii> RADII[] = {{P::BORDER_RADIUS, &S::borderRadius}};
        const Field<std::vector<Shadow>> SHADOWS[] = {{P::BOX_SHADOW, &S::shadows}};
        const Field<Transform> TRANSFORMS[] = {{P::TRANSFORM, &S::transform}};
        const Field<Vec2> ORIGINS[] = {{P::TRANSFORM_ORIGIN, &S::transformOrigin}};
        const Field<std::string> STRINGS[] = {{P::FONT_FAMILY, &S::fontFamily}};
        const Field<std::vector<TransitionSpec>> TRANSITIONS[] = {{P::TRANSITION, &S::transitions}};

        // Запись значения в поле, если свойство есть в таблице / Writing the value into the field if the property is in the table
        template <typename T, std::size_t N>
        bool assign(const Field<T> (&fields)[N], ComputedStyle& style, const Declaration& declaration)
        {
            const T* value = std::get_if<T>(&declaration.second);

            if (value == nullptr)
                return false;
            for (const Field<T>& field : fields)
            {
                if (field.id == declaration.first)
                {
                    style.*field.member = *value;
                    return true;
                }
            }
            return false;
        }
    }

    // Пробуем таблицы по очереди; тип значения отсекает лишние сразу
    // The tables are tried in turn; the value type rules out the wrong ones at once
    void applyDeclaration(ComputedStyle& style, const Declaration& declaration)
    {
        // Своё значение цвета обрывает наследование / An own color value breaks the inheritance
        if (declaration.first == PropertyId::COLOR)
            style.inheritedColors = static_cast<std::uint8_t>(style.inheritedColors & ~INHERITED_COLOR);
        else if (declaration.first == PropertyId::ACCENT_COLOR)
            style.inheritedColors = static_cast<std::uint8_t>(style.inheritedColors & ~INHERITED_ACCENT);
        bool done = assign(FLOATS, style, declaration) || assign(INTS, style, declaration)
            || assign(LENGTHS, style, declaration) || assign(INSETS, style, declaration)
            || assign(BOOLS, style, declaration) || assign(TRACKS, style, declaration)
            || assign(PAINTS, style, declaration) || assign(COLORS, style, declaration)
            || assign(RADII, style, declaration) || assign(SHADOWS, style, declaration)
            || assign(TRANSFORMS, style, declaration) || assign(ORIGINS, style, declaration)
            || assign(STRINGS, style, declaration) || assign(TRANSITIONS, style, declaration);

        // Неизвестное сочетание свойства и типа — ошибка программы, а не разметки
        // An unknown property and type combination is a program error, not a markup one
        (void)done;
    }

    // Наследуются только текстовые свойства, как в CSS / Only the text properties are inherited, as in CSS
    void inheritFrom(ComputedStyle& style, const ComputedStyle& parent)
    {
        style.color = parent.color;
        style.fontFamily = parent.fontFamily;
        style.fontSize = parent.fontSize;
        style.fontWeight = parent.fontWeight;
        style.fontStyle = parent.fontStyle;
        style.lineHeight = parent.lineHeight;
        style.letterSpacing = parent.letterSpacing;
        style.textAlign = parent.textAlign;
        style.textWrap = parent.textWrap;
        style.accentColor = parent.accentColor;
        style.inheritedColors = INHERITED_COLOR | INHERITED_ACCENT;
        style.cursor = parent.cursor;
    }
}
