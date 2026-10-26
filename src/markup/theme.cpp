#include "gran/markup/theme.hpp"

namespace gran::generated
{
    // Встроенная тема (gen/markup/default_theme.cpp) / The built-in theme (gen/markup/default_theme.cpp)
    extern const unsigned char DEFAULT_THEME[];
    extern const std::size_t DEFAULT_THEME_SIZE;
}

namespace gran::markup
{
    // Стили видов ищутся по именам элементов; атрибуты окна темы становятся правилом окна
    // The kind styles are looked up by element names; the theme window attributes become the window rule
    std::unique_ptr<Theme> loadTheme(std::string_view markup)
    {
        std::unique_ptr<Theme> theme = std::make_unique<Theme>();

        theme->document = loadMarkupString(markup, LoadOptions{});
        for (std::size_t kind = 0; kind < ui::ELEMENT_KIND_COUNT; ++kind)
            theme->kinds.rules[kind] = theme->document.styles.find(ui::elementName(static_cast<ui::ElementKind>(kind)));
        theme->window.name = "Window";
        theme->window.block = theme->document.window.declared;
        return theme;
    }

    // Локальная статическая переменная: потокобезопасная загрузка при первом обращении
    // A local static variable: thread-safe loading on first access
    const Theme& defaultTheme()
    {
        static const std::unique_ptr<Theme> THEME = loadTheme(std::string_view(
            reinterpret_cast<const char*>(generated::DEFAULT_THEME), generated::DEFAULT_THEME_SIZE));

        return *THEME;
    }

    // Окно не наследует ни от кого: умолчания темы, затем атрибуты приложения
    // The window inherits from nobody: the theme defaults, then the application attributes
    style::ComputedStyle windowStyle(const Theme& theme, const WindowSpec& window)
    {
        style::CascadeInput input;

        input.defaults = &theme.window;
        input.inlineBlock = &window.declared;
        return style::computeStyle(input);
    }
}
