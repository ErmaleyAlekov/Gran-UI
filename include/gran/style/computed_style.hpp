#ifndef GRAN_STYLE_COMPUTED_STYLE_HPP
#define GRAN_STYLE_COMPUTED_STYLE_HPP

#include "gran/style/property.hpp"

#include <limits>

namespace gran::style
{
    // Ориентация стека / The stack orientation
    enum class Orientation : std::uint8_t { HORIZONTAL, VERTICAL };

    // Распределение по главной оси / The main axis distribution
    enum class Justify : std::uint8_t { START, CENTER, END, SPACE_BETWEEN, SPACE_AROUND, SPACE_EVENLY };

    // Выравнивание по поперечной оси (AUTO — как у родителя) / The cross axis alignment (AUTO — as the parent says)
    enum class Align : std::uint8_t { START, CENTER, END, STRETCH, AUTO };

    // Выравнивание строк текста / The text line alignment
    enum class TextAlign : std::uint8_t { START, CENTER, END, JUSTIFY };

    // Что делать с содержимым за краем блока / What to do with the content beyond the box edge
    enum class Overflow : std::uint8_t { VISIBLE, HIDDEN };

    // Курсор мыши над элементом / The mouse cursor over the element
    enum class Cursor : std::uint8_t { DEFAULT, POINTER, TEXT, MOVE, NOT_ALLOWED, GRAB, EW_RESIZE, NS_RESIZE };

    // Биты ComputedStyle::inheritedColors / The ComputedStyle::inheritedColors bits
    enum InheritedColor : std::uint8_t
    {
        INHERITED_COLOR = 1,
        INHERITED_ACCENT = 2
    };

    // Перенос и обрезка текста / Text wrapping and truncation
    enum class TextWrap : std::uint8_t { WRAP, NOWRAP };
    enum class TextOverflow : std::uint8_t { CLIP, ELLIPSIS };

    // Линия оформления текста (text-decoration-line в CSS) / The text decoration line (text-decoration-line in CSS)
    enum class TextDecoration : std::uint8_t { NONE, UNDERLINE, LINE_THROUGH };

    // «Не задано» для left/top/right/bottom / "Not set" for left/top/right/bottom
    constexpr float UNSET = std::numeric_limits<float>::quiet_NaN();

    // Бесконечность для max-размеров / The infinity for the max sizes
    constexpr float UNBOUNDED = std::numeric_limits<float>::infinity();

    /*
    ** Вычисленный стиль узла: каждое свойство имеет окончательное значение
    ** (умолчание ← наследование ← стиль ← атрибуты ← состояния). Компоновка
    ** и отрисовка читают только эту структуру.
    **
    ** The computed style of a node: every property has a final value
    ** (default ← inheritance ← style ← attributes ← states). Layout
    ** and rendering read only this structure.
    */
    struct ComputedStyle
    {
        // ---- Блок / Box ----
        Length width;
        Length height;
        float minWidth = 0.0f;
        float maxWidth = UNBOUNDED;
        float minHeight = 0.0f;
        float maxHeight = UNBOUNDED;
        Insets margin;
        Insets padding;
        float grow = 0.0f;
        float shrink = 1.0f;
        int alignSelf = static_cast<int>(Align::AUTO);
        int gridRow = 0;
        int gridColumn = 0;
        int gridRowSpan = 1;
        int gridColumnSpan = 1;
        float left = UNSET;
        float top = UNSET;
        float right = UNSET;
        float bottom = UNSET;
        int zIndex = 0;

        // ---- Контейнер / Container ----
        int orientation = static_cast<int>(Orientation::VERTICAL);
        float spacing = 0.0f;
        int justify = static_cast<int>(Justify::START);
        int align = static_cast<int>(Align::STRETCH);
        bool wrap = false;
        std::vector<GridTrack> rows;
        std::vector<GridTrack> columns;
        float rowSpacing = 0.0f;
        float columnSpacing = 0.0f;
        int overflow = static_cast<int>(Overflow::VISIBLE);

        // ---- Вид / Visual ----
        Paint background;
        Color borderColor;
        Insets borderWidth;
        CornerRadii borderRadius;
        std::vector<Shadow> shadows;
        float opacity = 1.0f;
        Transform transform;
        Vec2 transformOrigin{0.5f, 0.5f};
        float backdropBlur = 0.0f;
        int cursor = static_cast<int>(Cursor::DEFAULT);
        // Затемнение окна под модальным слоем (Dialog); прозрачное — без затемнения
        // The window dimming under a modal layer (a Dialog); transparent — no dimming
        Color scrim;

        // ---- Текст / Text ----
        Color color{0.0f, 0.0f, 0.0f, 1.0f};
        std::string fontFamily;
        float fontSize = 14.0f;
        int fontWeight = 400;
        int fontStyle = 0;
        float lineHeight = 1.3f;
        float letterSpacing = 0.0f;
        int textAlign = static_cast<int>(TextAlign::START);
        int textWrap = static_cast<int>(TextWrap::WRAP);
        int textOverflow = static_cast<int>(TextOverflow::CLIP);
        int maxLines = 0;
        int textDecoration = static_cast<int>(TextDecoration::NONE);
        Color accentColor{0.145f, 0.388f, 0.922f, 1.0f};
        // Какие анимируемые цвета взяты у родителя (биты InheritedColor): переход цвета родителя
        // доходит до таких потомков каждый кадр, как наследование вычисленного значения в CSS
        // Which animatable colors come from the parent (InheritedColor bits): a parent color transition
        // reaches such descendants every frame, as the computed value inheritance in CSS
        std::uint8_t inheritedColors = 0;

        // ---- Движение / Motion ----
        std::vector<TransitionSpec> transitions;
    };
}

#endif
