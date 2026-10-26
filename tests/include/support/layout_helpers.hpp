#ifndef GRAN_TEST_LAYOUT_HELPERS_HPP
#define GRAN_TEST_LAYOUT_HELPERS_HPP

#include "gran/layout/layout.hpp"
#include "gran/markup/markup_loader.hpp"

namespace support
{
    /*
    ** Измеритель-заглушка: каждый символ (байт UTF-8) шириной fontSize / 2, строка высотой
    ** fontSize × lineHeight, перенос по ширине; картинки — 64×48. Результаты компоновки
    ** в тестах так не зависят от системных шрифтов.
    **
    ** A stub measurer: every character (UTF-8 byte) is fontSize / 2 wide, a line is
    ** fontSize × lineHeight high, wrapping by width; images are 64×48. Layout results
    ** in tests thus do not depend on system fonts.
    */
    class FixedMeasurer : public gran::layout::ContentMeasurer
    {
    public:
        gran::Size measureText(const gran::ui::Node& node, float maxWidth) override;
        gran::Size imageSize(const gran::ui::Node& node) override;
    };

    // Разметка → дерево со стилями темы по умолчанию → компоновка в окне width × height
    // Markup → a tree with the default theme styles → layout in a width × height window
    gran::markup::UiDocument layoutMarkup(const char* markup, float width, float height);

    // Узел по id (бросает std::runtime_error, если его нет) / A node by id (throws std::runtime_error if missing)
    const gran::ui::Node& nodeById(const gran::markup::UiDocument& document, const char* id);
}

#endif
