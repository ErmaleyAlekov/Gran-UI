#ifndef GRAN_MARKUP_THEME_HPP
#define GRAN_MARKUP_THEME_HPP

#include "gran/markup/markup_loader.hpp"
#include "gran/ui/style_pass.hpp"

#include <memory>
#include <string_view>

namespace gran::markup
{
    /*
    ** Тема: документ разметки, из которого взяты стили видов элементов (стиль с именем
    ** "Button" — для всех кнопок) и умолчания окна. Тема — тоже разметка: её проверяет
    ** та же схема XSD, а дизайнер меняет её без перекомпиляции.
    **
    ** A theme: a markup document from which the element kind styles are taken (the style named
    ** "Button" — for all buttons) and the window defaults. A theme is markup too: the same XSD schema
    ** checks it, and a designer changes it without recompiling.
    */
    struct Theme
    {
        UiDocument document;
        ui::KindStyles kinds;
        style::StyleRule window;
    };

    // ---- theme.cpp ----

    // Тема из текста разметки / A theme from a markup text
    std::unique_ptr<Theme> loadTheme(std::string_view markup);

    // Встроенная тема по умолчанию (загружается один раз) / The built-in default theme (loaded once)
    const Theme& defaultTheme();

    // Вычисленный стиль окна: умолчания темы, затем атрибуты <Window> приложения
    // The computed window style: the theme defaults, then the application <Window> attributes
    style::ComputedStyle windowStyle(const Theme& theme, const WindowSpec& window);
}

#endif
