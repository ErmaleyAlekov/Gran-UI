#ifndef GRAN_STYLE_VALUE_PARSERS_HPP
#define GRAN_STYLE_VALUE_PARSERS_HPP

#include "gran/style/values.hpp"

#include <string_view>
#include <vector>

namespace gran::style
{
    /*
    ** Разбор значений атрибутов. Синтаксис — как у CSS, чтобы веб-разработчику не учить
    ** новый; списки разделяются запятыми и/или пробелами. Ошибка — GranException
    ** с кодом STYLE_VALUE и текстом из каталога. Схема XSD проверяет лексику раньше,
    ** парсер — смысл (число опорных точек, диапазоны).
    **
    ** Attribute value parsing. The syntax is as in CSS so that a web developer does not
    ** learn a new one; lists are separated by commas and/or spaces. An error is a GranException
    ** with the STYLE_VALUE code and a text from the catalog. The XSD schema checks the lexis
    ** earlier, the parser checks the meaning (the stop count, the ranges).
    */

    // ---- parse_numbers.cpp ----

    // Число без единиц / A number without units
    float parseNumber(std::string_view text);

    // "auto", "120", "120px", "50%" / "auto", "120", "120px", "50%"
    Length parseLength(std::string_view text);

    // "200ms", "0.2s" → секунды / "200ms", "0.2s" → seconds
    float parseDuration(std::string_view text);

    // "45deg", "0.5turn", "1.2rad", "45" (градусы) → радианы / "45deg", "0.5turn", "1.2rad", "45" (degrees) → radians
    float parseAngle(std::string_view text);

    // "0.5" или "50%" → доля 0..1 / "0.5" or "50%" → a fraction 0..1
    float parseFraction(std::string_view text);

    // ---- parse_color.cpp ----

    // #rgb, #rgba, #rrggbb, #rrggbbaa, rgb(), rgba(), hsl(), hsla(), transparent, имена CSS
    // #rgb, #rgba, #rrggbb, #rrggbbaa, rgb(), rgba(), hsl(), hsla(), transparent, CSS names
    Color parseColor(std::string_view text);

    // ---- parse_box.cpp ----

    // Отступы 1–4 числа в порядке CSS / Insets of 1–4 numbers in the CSS order
    Insets parseInsets(std::string_view text);

    // Радиусы 1–4 числа: tl tr br bl, как border-radius / Radii of 1–4 numbers: tl tr br bl, as border-radius
    CornerRadii parseRadii(std::string_view text);

    // Числа через запятые и/или пробелы / Numbers separated by commas and/or spaces
    std::vector<float> parseNumberList(std::string_view text);

    // ---- parse_paint.cpp ----

    // Цвет или градиент; "none" — без заливки / A color or a gradient; "none" — no fill
    Paint parsePaint(std::string_view text);

    // Список теней через запятую; "none" — пустой / A comma separated shadow list; "none" — empty
    std::vector<Shadow> parseShadows(std::string_view text);

    // ---- parse_gradient.cpp ----

    // linear-gradient(), radial-gradient(), conic-gradient() / linear-gradient(), radial-gradient(), conic-gradient()
    Gradient parseGradient(std::string_view text);

    // ---- parse_motion.cpp ----

    // Кривая анимации / An animation curve
    Easing parseEasing(std::string_view text);

    // Список переходов через запятую / A comma separated transition list
    std::vector<TransitionSpec> parseTransitions(std::string_view text);

    // ---- parse_animation.cpp ----

    // Список анимаций по ключевым кадрам через запятую; "none" — пустой / A comma separated keyframe animation list; "none" — empty
    std::vector<AnimationSpec> parseAnimations(std::string_view text);

    // translate(), scale(), rotate(), skew() подряд; "none" — пустой
    // translate(), scale(), rotate(), skew() in a row; "none" — empty
    Transform parseTransform(std::string_view text);

    // ---- parse_tracks.cpp ----

    // Дорожки сетки: "48, auto, 1fr, minmax(120, 2fr)" / Grid tracks: "48, auto, 1fr, minmax(120, 2fr)"
    std::vector<GridTrack> parseTracks(std::string_view text);

    // ---- parse_keyword.cpp ----

    // Запись таблицы ключевых слов / A keyword table entry
    struct Keyword
    {
        const char* name;
        int value;
    };

    // Значение ключевого слова из таблицы; ошибка перечисляет допустимые слова
    // The keyword value from the table; the error lists the allowed words
    int parseKeyword(std::string_view text, const Keyword* table, std::size_t count);

    // "true" / "false" (и "1" / "0", как xs:boolean) / "true" / "false" (and "1" / "0", as xs:boolean)
    bool parseBoolean(std::string_view text);
}

#endif
