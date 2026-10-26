#include "gran/input/internal/text_cursor.hpp"

namespace gran::input::internal
{
    namespace
    {
        // Байт продолжения UTF-8 (10xxxxxx) / A UTF-8 continuation byte (10xxxxxx)
        bool isContinuation(char byte) noexcept
        {
            return (static_cast<unsigned char>(byte) & 0xC0) == 0x80;
        }
    }

    // Назад через байты продолжения / Backward through the continuation bytes
    std::size_t previousBoundary(const std::string& text, std::size_t position) noexcept
    {
        if (position == 0)
            return 0;
        --position;
        while (position > 0 && isContinuation(text[position]))
            --position;
        return position;
    }

    // Вперёд через байты продолжения / Forward through the continuation bytes
    std::size_t nextBoundary(const std::string& text, std::size_t position) noexcept
    {
        if (position >= text.size())
            return text.size();
        ++position;
        while (position < text.size() && isContinuation(text[position]))
            ++position;
        return position;
    }

    // Ширина текста влияет на компоновку (auto-размер), поэтому DIRTY_LAYOUT / The text width affects layout (an auto size), hence DIRTY_LAYOUT
    void textChanged(RouterState& state, ui::Node& input)
    {
        ui::markDirty(input, ui::DIRTY_LAYOUT | ui::DIRTY_PAINT);
        if (!input.content.onChange.empty())
            state.actions->action(input.content.onChange, input);
    }
}
