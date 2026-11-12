#include "gran/markup/internal/build_context.hpp"
#include "gran/style/value_parsers.hpp"

#include "beresta.hpp"

#include <cmath>
#include <sstream>

namespace gran::markup::internal
{
    namespace
    {
        // Логическое значение текстом разметки / A boolean as markup text
        const char* flagText(bool flag) noexcept
        {
            if (flag)
                return "true";
            return "false";
        }

        // Длины краёв «левый правый верхний нижний» через пробел (−1 — от панели) / The edge lengths "left right top bottom" by spaces (−1 — from the panel)
        void readSizes(ui::DockState& dock, const std::string& value)
        {
            std::istringstream list(value);
            std::string part;

            for (std::size_t side = 0; side < ui::DOCK_SIDES && list >> part; ++side)
                dock.sizes[side] = style::parseNumber(part);
        }
    }

    /*
    ** Атрибуты закрепляемых панелей. DockPanel: title (подпись заголовка), dock (left, right, top, bottom), size,
    ** active, hidden. DockHost: sizes (длины краёв — так их хранят настройки). false — не атрибут панелей.
    **
    ** The dockable panel attributes. DockPanel: title (the title label), dock (left, right, top, bottom), size,
    ** active, hidden. DockHost: sizes (the edge lengths — the settings keep them this way). false — not a panel attribute.
    */
    bool readDockAttribute(ui::Node& node, std::string_view name, const std::string& value)
    {
        static const style::Keyword SIDES[] = {{"left", 0}, {"right", 1}, {"top", 2}, {"bottom", 3}};
        ui::DockState& dock = node.content.dock;

        if (node.kind == ui::ElementKind::DOCK_HOST && name == "sizes")
            readSizes(dock, value);
        else if (node.kind != ui::ElementKind::DOCK_PANEL)
            return false;
        else if (name == "title")
            node.content.text = value;
        else if (name == "dock")
            dock.side = static_cast<ui::DockSide>(style::parseKeyword(value, SIDES));
        else if (name == "size")
            dock.size = style::parseNumber(value);
        else if (name == "active")
            dock.active = value == "true" || value == "1";
        else if (name == "hidden")
            dock.hidden = value == "true" || value == "1";
        else
            return false;
        return true;
    }

    /*
    ** Живая раскладка — в DOM (без перестройки): у панели край, выбранность и скрытость, у хоста длины краёв.
    ** Их меняет мышь, а настройки сохраняют атрибуты разметки.
    **
    ** The live layout goes into the DOM (without rebuilding): for a panel the edge, the choice and the hiddenness, for the host the edge lengths.
    ** The mouse changes them, while the settings keep the markup attributes.
    */
    void writeDockLive(const ui::Node& node, beresta::Element& element)
    {
        static const char* const SIDES[] = {"left", "right", "top", "bottom"};
        const ui::DockState& dock = node.content.dock;
        std::ostringstream sizes;

        if (node.kind == ui::ElementKind::DOCK_PANEL)
        {
            element.setAttribute("dock", SIDES[static_cast<std::size_t>(dock.side)]);
            element.setAttribute("active", flagText(dock.active));
            element.setAttribute("hidden", flagText(dock.hidden));
            return;
        }
        for (std::size_t side = 0; side < ui::DOCK_SIDES; ++side)
        {
            if (side > 0)
                sizes << ' ';
            sizes << std::round(dock.sizes[side]);
        }
        element.setAttribute("sizes", sizes.str());
    }

    // Что сохраняется без persist: раскладка закрепляемых панелей по их id / What is kept without persist: the dockable panel layout by their ids
    std::string dockPersistNames(const ui::Node& node)
    {
        if (node.kind == ui::ElementKind::DOCK_PANEL)
            return "dock active hidden";
        if (node.kind == ui::ElementKind::DOCK_HOST)
            return "sizes";
        return std::string();
    }
}
