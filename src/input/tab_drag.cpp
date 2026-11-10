#include "gran/input/input_router.hpp"

namespace gran::input
{
    namespace
    {
        // Точка окна над крестиком закрытия (место — как в tabHeaderSize и paintTabClose) / A window point over the close cross (the place is as in tabHeaderSize and paintTabClose)
        bool inCloseMark(const ui::Node& header, Vec2 point)
        {
            return ui::tabClosable(header) && closeMarkHit(header, point);
        }
    }

    /*
    ** Нажатие на заголовок вкладки: по крестику — запрос закрыть (без выбора вкладки: нажатие снимается), иначе
    ** у переставляемых вкладок начинается перетаскивание заголовка. Обработчик onClose может сразу удалить узлы —
    ** после запроса к заголовку больше не обращаемся.
    **
    ** A press on a tab header: on the cross — a request to close (without choosing the tab: the press is removed), otherwise
    ** reorderable tabs start dragging the header. The onClose handler may remove the nodes at once —
    ** after the request the header is not touched any more.
    */
    void pressTab(RouterState& state, ui::Node& header, const platform::Event& event)
    {
        ui::Node* tabs = ui::tabsOf(header);

        if (inCloseMark(header, event.position))
        {
            changePressed(state, nullptr);
            requestTabClose(state, header);
            return;
        }
        if (tabs == nullptr || !tabs->content.tabs.reorderable)
            return;
        state.drag = &header;
        state.tabMoved = false;
    }

    /*
    ** Перетаскивание заголовка: указатель зашёл за середину соседнего заголовка — вкладка встаёт на его место
    ** (за середину — чтобы заголовки разной ширины не менялись туда-обратно на каждом движении).
    **
    ** Dragging a header: the pointer went past the middle of a neighbor header — the tab takes its place
    ** (past the middle — so that headers of different widths do not swap back and forth on every move).
    */
    void dragTab(RouterState& state, ui::Node& header, Vec2 point)
    {
        ui::Node* tabs = ui::tabsOf(header);
        std::size_t from = ui::headerNumber(header);

        if (tabs == nullptr)
            return;
        Vec2 local = ui::windowToNode(*header.parent, point);
        for (std::size_t index = 0; index < header.parent->children.size(); ++index)
        {
            const ui::Node& other = *header.parent->children[index];
            float middle = other.layout.frame.x + other.layout.frame.width * 0.5f;
            if (other.kind != ui::ElementKind::TAB_HEADER || index == from)
                continue;
            bool passed = (index > from && local.x > middle) || (index < from && local.x < middle);
            if (passed && local.x >= other.layout.frame.x && local.x < other.layout.frame.x + other.layout.frame.width)
            {
                state.tabMoved = ui::moveTab(*tabs, from, index) || state.tabMoved;
                return;
            }
        }
    }

    // Конец перетаскивания: вкладка сдвинулась — onReorder у Tabs со сдвинутой страницей / The drag end: the tab moved — onReorder of Tabs with the moved page
    void finishTabDrag(RouterState& state, ui::Node& header)
    {
        ui::Node* tabs = ui::tabsOf(header);
        ui::Node* page = ui::pageOf(header);

        if (state.tabMoved && tabs != nullptr && page != nullptr && !tabs->content.tabs.onReorder.empty())
            state.actions->action(tabs->content.tabs.onReorder, *page);
        state.tabMoved = false;
    }

    // Средняя кнопка по заголовку закрываемой вкладки — запрос закрыть, как в браузерах / The middle button on a closable tab header — a close request, as in browsers
    void middleClick(RouterState& state, ui::Node* hit)
    {
        ui::Node* header = controlOf(hit);

        if (header != nullptr && ui::tabClosable(*header))
            requestTabClose(state, *header);
    }
}
