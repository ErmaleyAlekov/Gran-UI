#include "ui/controller.hpp"
namespace editor
{
    // Изменения проходят через dirty-флаги движка. / Changes use the engine's dirty flags.
    void Controller::setText(const std::string& id, const std::string& text)
    {
        auto* node = app.find(id);
        if (!node || node->content.text == text)
            return;
        node->content.text = text;
        gran::ui::markDirty(*node, gran::ui::DIRTY_LAYOUT | gran::ui::DIRTY_PAINT);
    }

    // Скрытые области не участвуют в компоновке. / Hidden areas do not participate in layout.
    void Controller::show(const std::string& id, bool visible)
    {
        auto* node = app.find(id);
        if (!node || node->visible == visible)
            return;
        node->visible = visible;
        gran::ui::markDirty(*node, gran::ui::DIRTY_LAYOUT | gran::ui::DIRTY_PAINT);
    }

    // Узел сначала присоединяется, затем получает стиль окна.
    // Attach the node before resolving its window's style.
    gran::ui::Node& Controller::add(gran::ui::Node& parent, gran::ui::ElementKind kind,
        const std::string& style, const std::string& text)
    {
        auto node = std::make_unique<gran::ui::Node>();
        node->kind = kind;
        node->content.text = text;
        auto& result = app.append(parent, std::move(node));
        app.setStyle(result, style);
        return result;
    }

    // Идентификаторы не зависят от положения вкладки. / Identifiers do not depend on tab position.
    Document* Controller::document(unsigned id)
    {
        for (auto& item : documents)
            if (item.id == id)
                return &item;
        return nullptr;
    }

    // Вложенные иконки относятся к той же вкладке. / Nested icons belong to the same tab.
    unsigned Controller::nodeDocument(const gran::ui::Node& node) const
    {
        for (auto* current = &node; current; current = current->parent)
            for (const auto& item : documents)
                if (current == item.page || current == item.tab || current == item.label || current == item.close)
                    return item.id;
        return active;
    }
}
