#include "text.hpp"
#include <filesystem>
#include <string_view>
#include <cctype>

namespace editor
{
    // Возвращает имя файла из полного пути. Возвращает пустую строку для пустого пути.
    // Returns the file name from the full path. Returns an empty string for an empty path.
    std::string fileLabel(const std::string& path)
    {
        if (path.empty())
            return "";
        auto fsPath = std::filesystem::u8path(path);
        return fsPath.filename().u8string();
    }

    // Возвращает метку языка на основе расширения файла.
    // Returns the language label based on the file extension.
    std::string languageLabel(const std::string& path)
    {
        if (path.empty())
            return "Plain Text";
        auto fsPath = std::filesystem::u8path(path);
        auto ext = fsPath.extension().string();
        if (ext == ".cpp" || ext == ".hpp" || ext == ".h" || ext == ".cc" || ext == ".cxx")
            return "C++";
        if (ext == ".c")
            return "C";
        if (ext == ".xml")
            return "XML";
        if (ext == ".py")
            return "Python";
        if (ext == ".js")
            return "JavaScript";
        if (ext == ".ts")
            return "TypeScript";
        if (ext == ".json")
            return "JSON";
        if (ext == ".md")
            return "Markdown";
        if (fsPath.filename() == "CMakeLists.txt")
            return "CMake";
        return "Plain Text";
    }

    // Возвращает метку курсора в формате "Ln N, Col M".
    // Returns the cursor label in the format "Ln N, Col M".
    std::string cursorLabel(std::string_view text, std::size_t caret)
    {
        if (caret > text.size())
            caret = text.size();

        std::size_t line = 1;
        std::size_t column = 1;
        for (std::size_t i = 0; i < caret; ++i) {
            if (text[i] == '\n') {
                ++line;
                column = 1;
            } else if (text[i] != '\r' && (static_cast<unsigned char>(text[i]) & 0xc0) != 0x80) {
                ++column;
            }
        }

        return "Ln " + std::to_string(line) + ", Col " + std::to_string(column);
    }
}
