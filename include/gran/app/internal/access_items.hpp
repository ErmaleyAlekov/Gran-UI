#ifndef GRAN_APP_INTERNAL_ACCESS_ITEMS_HPP
#define GRAN_APP_INTERNAL_ACCESS_ITEMS_HPP

#include "gran/a11y/access_tree.hpp"
#include "gran/binding/list_bindings.hpp"

#include <cstddef>

namespace gran::app
{
    struct AppWindow;

    /*
    ** Сведения о строках списков для доступности (паттерн Adapter: a11y::ItemSource поверх binding::ListBindings).
    ** Один на приложение: привязки списков общие у всех окон. Живёт в Application::State сразу после привязок —
    ** окна ОС (и их провайдеры доступности) закрываются в теле ~State, раньше, чем уходят привязки.
    **
    ** The list row facts for accessibility (the Adapter pattern: a11y::ItemSource over binding::ListBindings).
    ** One per application: the list bindings are shared by all the windows. It lives in Application::State right after the bindings —
    ** the OS windows (and their accessibility providers) close in the ~State body, before the bindings go away.
    */
    class AccessItems final : public a11y::ItemSource
    {
    public:
        // ---- access_items.cpp ----
        explicit AccessItems(const binding::ListBindings& lists) noexcept;

        std::size_t itemIndex(const ui::Node& item) const noexcept override;
        std::size_t itemCount(const ui::Node& list) const noexcept override;
        const ui::Node* itemAt(const ui::Node& list, std::size_t index) const noexcept override;
        std::uint64_t itemVersion(const ui::Node& list) const noexcept override;

        // ---- access_items_select.cpp ----
        bool multipleSelection(const ui::Node& list) const noexcept override;
        std::size_t itemDepth(const ui::Node& item) const noexcept override;

    private:
        const binding::ListBindings& _lists;
    };

    // ---- access_focus.cpp ----

    /*
    ** Узел фокуса для дикторов: у списка строк таблицы или дерева с выделением — его текущая строка (у таблицы —
    ** текущая ячейка этой строки), иначе сам узел фокуса ввода. Фокус ввода остаётся у списка (клавиши идут ему),
    ** а диктор читает строку, по которой ходит человек.
    **
    ** The focus node for screen readers: for a table or tree row list with a selection — its current row (for a table —
    ** the current cell of that row), otherwise the input focus node itself. The input focus stays at the list (the keys go to it),
    ** while the screen reader reads the row the person walks over.
    */
    const ui::Node* accessFocusOf(const AppWindow& win, const ui::Node* focus) noexcept;
}

#endif
