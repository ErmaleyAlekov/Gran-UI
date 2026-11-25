#include "gran/render/text_engine.hpp"
#include "gran/utils/utf8.hpp"

#include <string_view>

namespace gran::render
{
    // Маска действует только у непустого поля пароля.
    // Only a non-empty password input is masked.
    bool isPasswordMasked(const ui::Node& node) noexcept
    {
        return node.kind == ui::ElementKind::INPUT && node.content.password && !node.content.text.empty();
    }

    // Символы до байта offset, каждый — 3 байта точки / The characters before the byte offset, each is 3 bytes of the dot
    std::size_t toDisplayOffset(const ui::Node& node, std::size_t offset) noexcept
    {
        const std::string& text = node.content.text;

        if (!isPasswordMasked(node))
            return offset;
        if (offset > text.size())
            offset = text.size();
        return countUtf8(std::string_view(text.data(), offset)) * PASSWORD_MASK_BYTES;
    }

    // Байт показа / 3 = номер символа; шаг по символам UTF-8 текста до него / A display byte / 3 = the character number; stepping over UTF-8 characters up to it
    std::size_t toContentOffset(const ui::Node& node, std::size_t offset) noexcept
    {
        const std::string& text = node.content.text;
        std::size_t characters = offset / PASSWORD_MASK_BYTES;
        std::size_t position = 0;

        if (!isPasswordMasked(node))
            return offset;
        for (std::size_t index = 0; index < characters && position < text.size(); ++index)
            decodeUtf8(text, position);
        return position;
    }
}
