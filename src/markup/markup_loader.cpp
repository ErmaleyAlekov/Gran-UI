#include "gran/markup/internal/build_context.hpp"

#include "beresta.hpp"

#include <filesystem>
#include <memory>

namespace gran::markup
{
    /*
    ** DOM переносится в кучу до построения: узлы запоминают адреса элементов, и эти адреса
    ** должны быть окончательными (перенос документа после построения на них не влияет, но так
    ** нет даже повода об этом думать).
    **
    ** The DOM moves to the heap before building: the nodes remember the element addresses, and these addresses
    ** must be final (moving the document after building does not affect them, but this way
    ** there is no reason to even think about it).
    */
    UiDocument loadMarkupString(std::string_view text, const LoadOptions& options)
    {
        std::shared_ptr<beresta::Document> dom = std::make_shared<beresta::Document>(internal::parseMarkupText(text));

        return internal::buildDocument(std::move(dom), options);
    }

    // Каталог файла — база для относительных путей картинок и шрифтов
    // The file directory is the base for relative image and font paths
    UiDocument loadMarkupFile(const std::string& path, const LoadOptions& options)
    {
        LoadOptions fileOptions = options;

        if (fileOptions.baseDirectory.empty())
            fileOptions.baseDirectory = std::filesystem::path(path).parent_path().string();
        std::shared_ptr<beresta::Document> dom = std::make_shared<beresta::Document>(internal::parseMarkupFile(path));
        return internal::buildDocument(std::move(dom), fileOptions);
    }
}
