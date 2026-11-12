#include "gran/layout/internal/layout_internal.hpp"

#include <algorithm>

namespace gran::layout::internal
{
    /*
    ** Видимость заголовка меняется прямо в проходе компоновки, поэтому кэш полосы нужно сбросить здесь же:
    ** флаг DIRTY_LAYOUT ставится самой полосе и заголовку без подъёма к предкам (хост и так компонуется сейчас).
    ** Без этого панель, перенесённая на новый край, показывала содержимое, а её заголовок в полосе не появлялся:
    ** полоса того же размера брала прошлую раскладку (layout_reuse.cpp), в которой заголовок был скрыт.
    **
    ** The title visibility changes right in the layout pass, so the strip cache has to be reset right here:
    ** the DIRTY_LAYOUT flag is set on the strip itself and the title without climbing to the ancestors (the host is being laid out now anyway).
    ** Without it a panel moved to a new edge showed its content, but its title did not appear in the strip:
    ** the strip of the same size took the previous layout (layout_reuse.cpp), in which the title was hidden.
    */
    void syncDockTitles(ui::Node& strip, const std::vector<ui::Node*>& panels, const ui::Node* chosen)
    {
        const std::uint8_t changed = ui::DIRTY_LAYOUT | ui::DIRTY_PAINT;

        for (const std::unique_ptr<ui::Node>& title : strip.children)
        {
            ui::Node* panel = ui::dockPanelOf(*title);
            bool shown = std::find(panels.begin(), panels.end(), panel) != panels.end();

            if (title->visible != shown)
            {
                title->visible = shown;
                title->dirty = static_cast<std::uint8_t>(title->dirty | changed);
                strip.dirty = static_cast<std::uint8_t>(strip.dirty | changed);
            }
            ui::setChecked(*title, panel == chosen);
        }
    }
}
