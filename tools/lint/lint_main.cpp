#include "gran/core/i18n.hpp"
#include "gran/markup/customization.hpp"
#include "gran/markup/markup_loader.hpp"

#include <iostream>
#include <set>

namespace
{
    using Ids = std::set<std::string>;

    // Сверяем только адресуемые узлы документа / Compare only addressable document nodes
    void collectIds(const gran::ui::Node& node, Ids& ids, bool editableOnly)
    {
        using gran::markup::Customization;
        using gran::markup::permitsCustomization;

        if (node.domElement && !node.id.empty()
            && (!editableOnly || permitsCustomization(*node.domElement, Customization::MOVE)
                || permitsCustomization(*node.domElement, Customization::HIDE)
                || permitsCustomization(*node.domElement, Customization::INSERT)
                || permitsCustomization(*node.domElement, Customization::STYLE)))
            ids.insert(node.id);
        for (const auto& child : node.children)
            collectIds(*child, ids, editableOnly);
    }

    // Штатная загрузка проверяет и схему, и семантику палитры.
    // The normal loader validates both the schema and the palette semantics.
    Ids loadIds(const char* path, bool editableOnly)
    {
        auto document = gran::markup::loadMarkupFile(path, {});
        Ids ids;

        if (document.root)
            collectIds(*document.root, ids, editableOnly);
        return ids;
    }
}

// Код 2 означает неверный вызов, код 1 — ошибку разметки.
// Exit code 2 indicates incorrect usage; exit code 1 indicates invalid markup.
int main(int argc, char* argv[])
{
    using gran::i18n::Message;

    if (argc != 2 && (argc != 4 || std::string_view(argv[2]) != "--previous"))
    {
        std::cerr << gran::i18n::format(Message::LINT_USAGE) << '\n';
        return 2;
    }
    try
    {
        const Ids current = loadIds(argv[1], false);
        if (argc == 4)
            for (const auto& id : loadIds(argv[3], true))
                if (current.count(id) == 0)
                    std::cerr << gran::i18n::format(Message::LINT_REMOVED_ID, {id}) << '\n';
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
