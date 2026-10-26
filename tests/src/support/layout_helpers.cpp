#include "support/layout_helpers.hpp"
#include "gran/markup/theme.hpp"

#include <cmath>
#include <stdexcept>

namespace support
{
    // Ширина символа — половина кегля; при нехватке ширины — столько строк, сколько нужно
    // The character width is half the font size; when the width is short — as many lines as needed
    gran::Size FixedMeasurer::measureText(const gran::ui::Node& node, float maxWidth)
    {
        float character = node.computed.fontSize * 0.5f;
        float total = character * static_cast<float>(node.content.text.size());
        float line = node.computed.fontSize * node.computed.lineHeight;

        if (!std::isfinite(maxWidth) || total <= maxWidth)
            return gran::Size{total, line};
        float lines = std::ceil(total / std::fmax(character, maxWidth));
        return gran::Size{maxWidth, lines * line};
    }

    // Картинка любого источника — 64×48 / An image of any source is 64×48
    gran::Size FixedMeasurer::imageSize(const gran::ui::Node& node)
    {
        (void)node;
        return gran::Size{64.0f, 48.0f};
    }

    // Полный конвейер без отрисовки / The full pipeline without rendering
    gran::markup::UiDocument layoutMarkup(const char* markup, float width, float height)
    {
        gran::markup::UiDocument document = gran::markup::loadMarkupString(markup, gran::markup::LoadOptions{});
        const gran::markup::Theme& theme = gran::markup::defaultTheme();
        FixedMeasurer measurer;

        gran::ui::resolveStyles(*document.root, gran::markup::windowStyle(theme, document.window), theme.kinds);
        gran::layout::layoutTree(*document.root, gran::Size{width, height}, measurer);
        return document;
    }

    // Узел по id / A node by id
    const gran::ui::Node& nodeById(const gran::markup::UiDocument& document, const char* id)
    {
        const gran::ui::Node* node = gran::ui::findNode(*document.root, id);

        if (node == nullptr)
            throw std::runtime_error(std::string("no node with id ") + id);
        return *node;
    }
}
