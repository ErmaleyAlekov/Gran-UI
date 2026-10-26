#include "gran/markup/internal/build_context.hpp"

#include "beresta.hpp"

#include <filesystem>

namespace gran::markup
{
    // Разбор и проверка текста, затем построение дерева / Parsing and checking a text, then building the tree
    UiDocument loadMarkupString(std::string_view text, const LoadOptions& options)
    {
        beresta::Document dom = internal::parseMarkupText(text);

        return internal::buildDocument(dom, options);
    }

    // Каталог файла — база для относительных путей картинок и шрифтов
    // The file directory is the base for relative image and font paths
    UiDocument loadMarkupFile(const std::string& path, const LoadOptions& options)
    {
        LoadOptions fileOptions = options;

        if (fileOptions.baseDirectory.empty())
            fileOptions.baseDirectory = std::filesystem::path(path).parent_path().string();
        beresta::Document dom = internal::parseMarkupFile(path);
        return internal::buildDocument(dom, fileOptions);
    }
}
