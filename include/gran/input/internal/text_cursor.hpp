#ifndef GRAN_INPUT_INTERNAL_TEXT_CURSOR_HPP
#define GRAN_INPUT_INTERNAL_TEXT_CURSOR_HPP

#include "gran/input/input_router.hpp"

#include <string>

namespace gran::input::internal
{
    // ---- text_cursor.cpp ----

    // Начало предыдущего символа UTF-8 / The start of the previous UTF-8 character
    std::size_t previousBoundary(const std::string& text, std::size_t position) noexcept;

    // Начало следующего символа UTF-8 / The start of the next UTF-8 character
    std::size_t nextBoundary(const std::string& text, std::size_t position) noexcept;

    // Текст поля изменился: перекомпоновка, перерисовка, onChange / The input text changed: relayout, repaint, onChange
    void textChanged(RouterState& state, ui::Node& input);

    // ---- text_clipboard.cpp ----

    // Ctrl+X / Ctrl+V поля ввода; true — что-то изменилось / Ctrl+X / Ctrl+V of an input; true — something changed
    bool clipboardEdit(RouterState& state, ui::Node& input, platform::Key key);

    // Выделенный текст в буфер обмена (не из поля пароля); true — скопировано / The selected text to the clipboard (not from a password input); true — copied
    bool copySelection(RouterState& state, const ui::Node& node);

    // ---- text_selection.cpp ----

    // Границы непустого выделения [from, to); false — выделения нет / The bounds of a non-empty selection [from, to); false — no selection
    bool selectionRange(const ui::NodeContent& content, std::size_t& from, std::size_t& to) noexcept;

    // Каретка в position; без extend выделение схлопывается / The caret to position; without extend the selection collapses
    void placeCaret(ui::Node& node, std::size_t position, bool extend);

    // Удаление выделенного текста поля; false — нечего удалять или только чтение / Erasing the selected input text; false — nothing to erase or read-only
    bool eraseSelection(RouterState& state, ui::Node& input);

    // Выделение слова (или пробелов, или знаков) вокруг байта position / Selecting the word (or the spaces, or the marks) around the byte position
    void selectWord(ui::Node& node, std::size_t position);

    // ---- text_words.cpp ----

    // Границы слова вокруг байта position: подряд идущие символы одного класса / The word bounds around the byte position: adjacent characters of one class
    void wordBounds(const std::string& text, std::size_t position, std::size_t& from, std::size_t& to);
}

#endif
