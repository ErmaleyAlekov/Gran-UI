#include "gran/app/internal/app_state.hpp"

#include <exception>

namespace gran::app
{
    namespace
    {
        // Ближайший всплывающий слой узла без снятия const (ui::popupOf принимает изменяемый узел; ревью Т-69) / The nearest popup layer of the node without dropping const (ui::popupOf takes a mutable node; review T-69)
        const ui::Node* layerOf(const ui::Node* node) noexcept
        {
            for (const ui::Node* current = node; current != nullptr; current = current->parent)
            {
                if (ui::isLayer(*current))
                    return current;
            }
            return nullptr;
        }
    }

    /*
    ** Удаляется поддерево с редактором (строку убрал виртуальный список, модель или код): набранное значение
    ** запоминается и запишется в безопасной точке кадра. Не удалось даже скопировать текст (нехватка памяти) —
    ** правка отменяется. Удаляется таблица — ссылки на неё снимаются; запись значения от узлов не зависит.
    **
    ** A subtree with the editor is removed (the row was removed by a virtual list, the model or the code): the typed value
    ** is remembered and will be written at the frame safe point. Even copying the text failed (out of memory) —
    ** the edit is cancelled. The table is removed — the references to it are dropped; writing the value does not depend on the nodes.
    */
    void gridEditRemoving(AppWindow& win, const ui::Node& node) noexcept
    {
        if (!win.gridEdit)
            return;
        GridEdit& edit = *win.gridEdit;
        if (edit.editor != nullptr && insideNode(edit.editor, &node))
        {
            edit.pending = GridEditEnd::COMMIT;
            try
            {
                edit.lostValue = readEditorValue(*edit.value);
                edit.lost = true;
            }
            catch (const std::exception&)
            {
                edit.pending = GridEditEnd::CANCEL;
            }
            edit.editor = nullptr;
            edit.value = nullptr;
            edit.hidden.clear();
        }
        // Спрятанный ребёнок ячейки ушёл сам (код программы) — возвращать его некуда / A hidden cell child went away by itself (the program code) — there is nowhere to bring it back
        for (std::size_t index = edit.hidden.size(); index > 0; --index)
        {
            if (insideNode(edit.hidden[index - 1], &node))
                edit.hidden.erase(edit.hidden.begin() + static_cast<std::ptrdiff_t>(index - 1));
        }
        if (insideNode(edit.grid, &node))
        {
            edit.grid = nullptr;
            edit.rows = nullptr;
        }
    }

    /*
    ** Фокус в списке строк, в другом поле или нигде — правка заканчивается записью. Фокус во всплывающем слое, который
    ** не содержит редактор (раскрытый список ComboBox из <EditTemplate>, меню), — правка продолжается: слой закроется
    ** и вернёт фокус туда, где он был. Слой, внутри которого стоит сама таблица (диалог), уходом не считается.
    **
    ** The focus in the row list, in another field or nowhere — the edit ends with writing. The focus in a popup layer that
    ** does not hold the editor (an open ComboBox list from <EditTemplate>, a menu) — the edit goes on: the layer will close
    ** and return the focus where it was. A layer the table itself stands in (a dialog) does not count as leaving.
    */
    void gridEditFocusChanged(AppWindow& win) noexcept
    {
        if (!win.gridEdit || !win.router)
            return;
        GridEdit& edit = *win.gridEdit;
        const ui::Node* focus = win.router->focused();
        if (edit.pending != GridEditEnd::NONE || edit.editor == nullptr || insideNode(focus, edit.editor))
            return;
        const ui::Node* layer = layerOf(focus);
        if (layer != nullptr && !insideNode(edit.editor, layer))
            return;
        edit.pending = GridEditEnd::COMMIT;
    }

    bool insideNode(const ui::Node* node, const ui::Node* subtree) noexcept
    {
        if (subtree == nullptr)
            return false;
        for (const ui::Node* current = node; current != nullptr; current = current->parent)
        {
            if (current == subtree)
                return true;
        }
        return false;
    }
}
