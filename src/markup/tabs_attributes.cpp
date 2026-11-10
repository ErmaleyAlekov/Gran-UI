#include "gran/markup/internal/build_context.hpp"

namespace gran::markup::internal
{
    /*
    ** Атрибуты вкладок документов у Tabs: closable, reorderable, overflowMenu, onClose, onReorder. Читаются раньше
    ** общей таблицы полей: closable у Dialog по умолчанию истинно, а у вкладок — ложно, поэтому у Tabs своё поле.
    ** false — не атрибут вкладок (или узел не Tabs).
    **
    ** The document tab attributes of Tabs: closable, reorderable, overflowMenu, onClose, onReorder. They are read before
    ** the common field table: closable of a Dialog is true by default, while for tabs it is false, so Tabs has its own field.
    ** false — not a tab attribute (or the node is not Tabs).
    */
    bool readTabsAttribute(ui::Node& node, std::string_view name, const std::string& value)
    {
        ui::TabsState& tabs = node.content.tabs;
        bool flag = value == "true" || value == "1";

        if (node.kind != ui::ElementKind::TABS)
            return false;
        if (name == "closable")
            tabs.closable = flag;
        else if (name == "reorderable")
            tabs.reorderable = flag;
        else if (name == "overflowMenu")
            tabs.overflowMenu = flag;
        else if (name == "onClose")
            tabs.onClose = value;
        else if (name == "onReorder")
            tabs.onReorder = value;
        else
            return false;
        return true;
    }
}
