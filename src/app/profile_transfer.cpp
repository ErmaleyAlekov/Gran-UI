#include "gran/app/internal/profile_transfer.hpp"

namespace gran::app
{
    namespace
    {
        // Заголовки вкладок пересоздаются при переносе, поэтому фокус ищется по значению.
        // Tab transfer rebuilds headers, so recover the focused header by value.
        void restoreChoice(ui::Node& node, const std::string& selected, AppWindow& next)
        {
            if (node.kind != ui::ElementKind::TABS)
            {
                ui::selectOption(node, ui::findOption(node, selected));
                return;
            }
            const bool headerFocus = next.pendingFocus && next.pendingFocus->kind == ui::ElementKind::TAB_HEADER
                && next.pendingFocus->parent && next.pendingFocus->parent->parent == &node;
            const std::string focusValue = headerFocus ? ui::optionValue(*next.pendingFocus) : std::string{};
            node.content.option = selected;
            ui::syncTabs(node);
            if (!headerFocus) return;
            next.pendingFocus = nullptr;
            for (const auto& header : node.children.front()->children)
                if (ui::optionValue(*header) == focusValue) next.pendingFocus = header.get();
        }

        // У страницы с id контейнер после Move может смениться, у созданной приложением остаётся место переноса.
        // A moved page may have a new owner; a runtime page has a matching transfer slot.
        void restoreHeader(const ui::Node& focus, AppWindow& next, const std::vector<ProfileTransfer>& plan)
        {
            if (!focus.parent) return;
            for (const auto& oldHeader : focus.parent->children)
            {
                if (oldHeader.get() != &focus) continue;
                auto* page = ui::pageOf(*oldHeader);
                if (!page) return;
                auto* target = page->id.empty() ? nullptr : ui::findNode(*next.document.root, page->id);
                for (const auto& item : plan)
                    if (item.child == page) target = item.parent->children[item.slot].get();
                auto* host = target ? target->parent : profileHost(*page->parent, *next.document.root);
                if (!host || host->kind != ui::ElementKind::TABS || host->children.empty()) return;
                for (const auto& header : host->children.front()->children)
                    if (target ? ui::pageOf(*header) == target : ui::optionValue(*header) == ui::optionValue(focus))
                    {
                        next.pendingFocus = header.get();
                        return;
                    }
            }
        }
    }

    // Контейнер разметки ищем по id, внутренний список ComboBox — через его хозяина.
    // Find a markup host by ID and an internal ComboBox list through its owner.
    ui::Node* profileHost(ui::Node& parent, ui::Node& root)
    {
        if (!parent.parent) return &root;
        if (!parent.id.empty()) return ui::findNode(root, parent.id);
        if (parent.parent->kind == ui::ElementKind::COMBO_BOX && !parent.parent->id.empty())
            if (auto* combo = ui::findNode(root, parent.parent->id))
                return ui::comboList(*combo);
        return nullptr;
    }

    // Создаём места для переноса до изменения старого дерева, включая заголовки динамических вкладок.
    // Prepare transfer slots before changing the old tree, including headers for dynamic tabs.
    void prepareProfileSlots(std::vector<ProfileTransfer>& plan, AppWindow& next, const ui::Node* focus)
    {
        std::map<ui::Node*, std::string> choices;
        for (auto& item : plan)
        {
            auto& host = *item.parent;
            auto stub = std::make_unique<ui::Node>();
            stub->id = item.child->id;
            stub->kind = item.child->kind;
            stub->enabled = item.child->enabled;
            if (host.kind == ui::ElementKind::TABS || (host.parent && host.parent->kind == ui::ElementKind::COMBO_BOX))
            {
                stub->content = item.child->content;
                auto* source = item.child->parent;
                if (host.kind == ui::ElementKind::TABS) choices[&host] = source->content.option;
                else choices[host.parent] = source->parent->content.option;
            }
            item.slot = host.children.size();
            ui::appendChild(host, std::move(stub));
        }
        for (const auto& [node, selected] : choices)
            restoreChoice(*node, selected, next);
        if (focus && focus->kind == ui::ElementKind::TAB_HEADER) restoreHeader(*focus, next, plan);
    }
}
