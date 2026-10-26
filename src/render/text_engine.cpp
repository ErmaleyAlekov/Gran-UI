#include "gran/render/text_engine.hpp"

#include <cmath>

namespace gran::render
{
    // Движок ссылается на библиотеку шрифтов окна / The engine refers to the window font library
    TextEngine::TextEngine(text::FontLibrary& fonts)
        : _fonts(fonts)
    {
    }

    // Размер — ширина самой длинной строки и высота всех строк / The size is the longest line width and all lines height
    Size TextEngine::measureText(const ui::Node& node, float maxWidth)
    {
        const text::TextLayout& layout = layoutFor(node, maxWidth);
        Size size{std::ceil(layout.width), layout.height};

        // Пустой текст (и пустое поле ввода) всё равно занимает одну строку
        // An empty text (and an empty input) still takes one line
        if (layout.lines.empty())
            size.height = node.computed.fontSize * node.computed.lineHeight;
        return size;
    }

    // Запись кэша создаётся при первом обращении (участками — если у Text есть Span) и помечается текущим кадром
    // A cache entry is created on first access (by runs — if the Text has Spans) and marked with the current frame
    const text::TextLayout& TextEngine::layoutFor(const ui::Node& node, float maxWidth)
    {
        text::TextStyle style = textStyleOf(node);
        const std::string& content = displayedText(node);
        std::vector<text::TextRun> runs = textRunsOf(node);
        std::string key = layoutKey(content, style, maxWidth) + runsKey(runs);
        auto found = _cache.find(key);

        if (found == _cache.end() && runs.empty())
            found = _cache.emplace(key, Entry{text::layoutText(content, style, maxWidth, _fonts), _frame}).first;
        else if (found == _cache.end())
            found = _cache.emplace(key, Entry{text::layoutRuns(content, runs, style, maxWidth, _fonts), _frame}).first;
        found->second.frame = _frame;
        return found->second.layout;
    }

    // Записи, не тронутые в этом кадре, удаляются / The entries not touched in this frame are removed
    void TextEngine::endFrame()
    {
        for (auto entry = _cache.begin(); entry != _cache.end();)
        {
            if (entry->second.frame != _frame)
                entry = _cache.erase(entry);
            else
                ++entry;
        }
        ++_frame;
    }
}
