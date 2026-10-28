#include "gran/render/content_sizer.hpp"

namespace gran::render
{
    // Ссылки на движок текста и хранилище картинок окна / References to the window text engine and image store
    ContentSizer::ContentSizer(TextEngine& text, ImageStore& images)
        : _text(text), _images(images)
    {
    }

    // Текст меряет движок текста / The text engine measures text
    Size ContentSizer::measureText(const ui::Node& node, float maxWidth)
    {
        return _text.measureText(node, maxWidth);
    }

    // Нечитаемая картинка — 0×0 (блок схлопывается, если размер не задан явно) / An unreadable image is 0×0 (the box collapses unless sized explicitly)
    Size ContentSizer::imageSize(const ui::Node& node)
    {
        if (node.kind == ui::ElementKind::ICON)
        {
            const vector::Shape* shape = iconShape(node, _images);
            if (shape == nullptr)
                return Size{0.0f, 0.0f};
            return Size{shape->viewBox.width, shape->viewBox.height};
        }
        const ImageEntry& entry = _images.get(node.content.source);

        if (entry.failed)
            return Size{0.0f, 0.0f};
        return Size{static_cast<float>(entry.width), static_cast<float>(entry.height)};
    }

    // Фигура из разметки важнее файла (схема разрешает только одно из двух) / The markup shape wins over the file (the schema allows only one of the two)
    const vector::Shape* iconShape(const ui::Node& node, ImageStore& images)
    {
        if (node.content.shape)
            return node.content.shape.get();
        if (node.content.source.empty())
            return nullptr;
        return images.shape(node.content.source);
    }
}
