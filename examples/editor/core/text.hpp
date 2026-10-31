#ifndef GRAN_EDITOR_TEXT_HPP
#define GRAN_EDITOR_TEXT_HPP
#include <cstddef>
#include <string>
#include <string_view>
namespace editor
{
    // Небольшие функции представления документа. / Small document presentation helpers.
    std::string fileLabel(const std::string& path);
    std::string languageLabel(const std::string& path);
    std::string cursorLabel(std::string_view text, std::size_t caret);
}
#endif
