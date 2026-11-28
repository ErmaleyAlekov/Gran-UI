#include "gran/app/internal/profile_transfer.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

#include <algorithm>
#include <set>

namespace gran::app
{
    namespace
    {
        // Коллизия с динамическим узлом прерывает подготовку до перемещения первого владельца.
        // An ID collision with a dynamic node aborts preparation before ownership is moved.
        void indexIds(const ui::Node& node, std::set<std::string>& ids, bool rejectExisting)
        {
            if (!node.id.empty() && !ids.insert(node.id).second && rejectExisting)
                throw GranException(ErrorCode::INVALID_ARGUMENT, i18n::format(i18n::Message::OVERLAY_DENIED, {node.id}));
            for (const auto& child : node.children)
                indexIds(*child, ids, rejectExisting);
        }

        // Сначала находим все будущие контейнеры, пока старое окно полностью работоспособно.
        // Locate every destination while the old window is still fully operational.
        void planTransfer(ui::Node& parent, ui::Node& root, std::vector<ProfileTransfer>& plan, std::set<std::string>& ids)
        {
            ui::Node* destination = nullptr;
            for (const auto& child : parent.children)
            {
                if (child->layoutInspector)
                {
                    if (child->visible) plan.push_back({child.get(), &root});
                    continue;
                }
                if (!child->appendedByApplication)
                {
                    planTransfer(*child, root, plan, ids);
                    continue;
                }
                destination = profileHost(parent, root);
                if (!destination || destination->kind != parent.kind)
                    throw GranException(ErrorCode::INVALID_ARGUMENT, i18n::format(i18n::Message::OVERLAY_DENIED, {parent.id}));
                indexIds(*child, ids, true);
                plan.push_back({child.get(), destination});
            }
        }

        // Перед освобождением старой таблицы стилей заменяем указатели правилами новой.
        // Replace rule pointers with entries in the new stylesheet before releasing the old one.
        void refreshRules(ui::Node& node, const style::StyleSheet& styles)
        {
            if (node.rule)
                node.rule = styles.find(node.rule->name);
            node.dirty |= ui::DIRTY_STYLE | ui::DIRTY_LAYOUT | ui::DIRTY_PAINT;
            for (const auto& child : node.children)
                refreshRules(*child, styles);
        }
    }

    // Добавленные из кода вкладки требуют сохранения адресов при смене профиля.
    // Tabs added from code need stable addresses when switching profiles.
    bool hasApplicationChildren(const ui::Node& node)
    {
        for (const auto& child : node.children)
            if (child->layoutInspector ? child->visible : child->appendedByApplication || hasApplicationChildren(*child))
                return true;
        return false;
    }

    // Подготовленные места принимают существующие узлы вместе с обработчиками и историей.
    // Prepared slots receive the existing nodes together with their listeners and history.
    void moveApplicationChildren(AppWindow& current, AppWindow& next)
    {
        std::vector<ProfileTransfer> plan;
        std::set<std::string> ids;
        indexIds(*next.document.root, ids, false);
        planTransfer(*current.document.root, *next.document.root, plan, ids);
        auto* focus = current.router ? current.router->focused() : current.pendingFocus;
        prepareProfileSlots(plan, next, focus);
        for (auto [child, parent, targetSlot] : plan)
        {
            auto& children = child->parent->children;
            auto slot = std::find_if(children.begin(), children.end(), [target = child](const auto& item) { return item.get() == target; });
            for (auto* ancestor = focus; ancestor; ancestor = ancestor->parent)
                if (ancestor == child)
                    next.pendingFocus = const_cast<ui::Node*>(focus);
            refreshRules(*child, next.document.styles);
            if (parent->kind == ui::ElementKind::TABS && child->kind == ui::ElementKind::TAB)
                child->visible = parent->children[targetSlot]->visible;
            child->parent = parent;
            parent->children[targetSlot] = std::move(*slot);
            children.erase(slot);
            ui::markDirty(*parent, ui::DIRTY_STYLE | ui::DIRTY_LAYOUT | ui::DIRTY_PAINT);
        }
    }
}
