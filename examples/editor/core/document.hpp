#ifndef GRAN_EDITOR_DOCUMENT_HPP
#define GRAN_EDITOR_DOCUMENT_HPP
#include "gran/ui/node.hpp"
#include <string>
namespace editor
{
    // Страница живёт до закрытия: курсор, прокрутка и undo не теряются при переключении.
    // The page lives until closed: switching preserves its cursor, scrolling and undo.
    struct Document
    {
        unsigned id = 0;
        std::string path;
        std::string name;
        std::string saved;
        gran::ui::Node* page = nullptr;
        gran::ui::Node* tab = nullptr;
        gran::ui::Node* label = nullptr;
        gran::ui::Node* close = nullptr;
    };
    // Файлы ограничены UTF-8 и 4 МиБ на первом этапе редактора.
    // Files are limited to UTF-8 and 4 MiB in this first editor iteration.
    bool readDocument(const std::string& path, std::string& text);
    bool writeDocument(const std::string& path, const std::string& text);
    std::string canonicalPath(const std::string& path);
}
#endif
