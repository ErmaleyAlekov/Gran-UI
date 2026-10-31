#include "document.hpp"
#include "gran/utils/utf8.hpp"
#include <filesystem>
#include <fstream>

namespace editor
{
    // Канонический путь устраняет повторное открытие относительных путей и ссылок.
    // Canonical paths prevent opening relative aliases and symlinks twice.
    std::string canonicalPath(const std::string& path)
    {
        std::error_code error;
        auto result = std::filesystem::weakly_canonical(std::filesystem::u8path(path), error);
        if (error)
            return path;
        return result.u8string();
    }

    // Проверяем размер, бинарные данные и UTF-8 до передачи в текстовое поле.
    // Check size, binary data and UTF-8 before passing data to the text field.
    bool readDocument(const std::string& path, std::string& text)
    {
        std::ifstream input(std::filesystem::u8path(path), std::ios::binary | std::ios::ate);
        if (!input)
            return false;
        auto size = input.tellg();
        if (size < 0 || size > 4 * 1024 * 1024)
            return false;
        std::string bytes(static_cast<std::size_t>(size), '\0');
        input.seekg(0);
        if (!input.read(bytes.data(), static_cast<std::streamsize>(bytes.size())))
            return false;
        for (std::size_t position = 0; position < bytes.size();)
        {
            std::size_t before = position;
            char32_t code = gran::decodeUtf8(bytes, position);
            if (code == 0 || (code == gran::REPLACEMENT_CHARACTER && position - before == 1))
                return false;
        }
        text = std::move(bytes);
        return true;
    }
}
