#ifndef GRAN_APP_INTERNAL_GRID_EDIT_HPP
#define GRAN_APP_INTERNAL_GRID_EDIT_HPP

#include "gran/data/list_source.hpp"
#include "gran/data/record_tracker.hpp"
#include "gran/platform/window.hpp"
#include "gran/ui/data_grid.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace gran::app
{
    struct AppWindow;

    /*
    ** Чем закончить правку в безопасной точке. SUBMIT — клавиша редактора (Enter, Tab): значение с ошибкой оставляет
    ** редактор открытым; COMMIT — фокус ушёл, строку убрали, окно закрывают: значение с ошибкой отменяется; CANCEL — Esc.
    **
    ** How to finish the edit at the safe point. SUBMIT — an editor key (Enter, Tab): a value with an error keeps
    ** the editor open; COMMIT — the focus left, the row was removed, the window is closing: a value with an error is cancelled; CANCEL — Esc.
    */
    enum class GridEditEnd : std::uint8_t { NONE, SUBMIT, COMMIT, CANCEL };

    // Куда перейти после записи по Tab / Where to go after writing by Tab
    enum class GridTab : std::uint8_t { NONE, FORWARD, BACKWARD };

    /*
    ** Правка одной ячейки таблицы (пункт 2.2.4). Редактор — узел внутри ячейки строки: прежнее содержимое
    ** ячейки скрыто на время правки (hidden), редактор растянут на всю ячейку. Запись отслеживается
    ** RecordTracker: пока редактор открыт, источник могут отсортировать, отфильтровать, дополнить — значение
    ** уйдёт в ту же запись. В окне не больше одной правки: у редактора фокус, а фокус один.
    **
    ** Указатели на узлы невладеющие и обнуляются при удалении поддерева (gridEditRemoving): строку с редактором
    ** может убрать виртуальный список (прокрутка), смена модели или код программы. Тогда набранное значение
    ** сохраняется в lostValue и записывается в безопасной точке кадра (settleGridEdit).
    **
    ** Правка заканчивается (редактор удаляется, модель меняется) только в безопасной точке — до и после обработки
    ** события маршрутизатором ввода и после подгонки кадра. Клавиши редактора, смена фокуса и удаление строки лишь
    ** ставят pending: они приходят из середины работы маршрутизатора, и перестройка дерева там оставила бы на его стеке
    ** висячие указатели (ревью Т-69).
    **
    ** One table cell edit (item 2.2.4). The editor is a node inside the row cell: the former cell content
    ** is hidden during the edit (hidden), the editor is stretched over the whole cell. The record is tracked by
    ** a RecordTracker: while the editor is open, the source may be sorted, filtered, extended — the value
    ** goes into the same record. A window has at most one edit: the editor has the focus, and the focus is single.
    **
    ** The node pointers are non-owning and are nulled when a subtree is removed (gridEditRemoving): the row with the editor
    ** may be removed by a virtual list (scrolling), a model change or the program code. Then the typed value
    ** is kept in lostValue and written at the frame safe point (settleGridEdit).
    **
    ** The edit finishes (the editor is removed, the model changes) only at a safe point — before and after the input router
    ** handles an event and after the frame fitting. The editor keys, a focus change and a row removal only set
    ** pending: they come from the middle of the router work, and rebuilding the tree there would leave dangling pointers
    ** on its stack (review T-69).
    */
    struct GridEdit
    {
        ui::Node* grid = nullptr;
        ui::Node* rows = nullptr;
        ui::Node* editor = nullptr;
        ui::Node* value = nullptr;
        std::vector<ui::Node*> hidden;
        std::shared_ptr<data::RecordTracker> record;
        std::string field;
        std::string key;
        data::ColumnType type = data::ColumnType::TEXT;
        std::string original;
        std::string lostValue;
        bool lost = false;
        // Стек отмены таблицы (undoStack) и ведёт ли она историю (undo) / The table undo stack (undoStack) and whether it keeps a history (undo)
        std::string undoStack;
        bool undoable = true;
        GridEditEnd pending = GridEditEnd::NONE;
        GridTab tab = GridTab::NONE;
    };

    // Где открывается редактор: таблица, список строк, ячейка, источник и номер записи / Where the editor opens: the table, the row list, the cell, the source and the record index
    struct GridEditPlace
    {
        ui::Node* grid;
        ui::Node* rows;
        ui::Node* cell;
        data::ListSource* source;
        std::size_t index;
    };

    // ---- grid_edit_begin.cpp ----

    /*
    ** Открыть редактор текущей ячейки таблицы, чей список строк — rows. typed — набранный символ, с которого
    ** начинается правка (значение заменяется им), nullptr — правка прежнего значения (F2, Enter, двойной щелчок).
    ** false — правка невозможна: не таблица, правка запрещена, нет текущей строки или столбец не правится.
    **
    ** Open the editor of the current table cell whose row list is rows. typed is the typed character the edit
    ** starts with (it replaces the value), nullptr — editing the former value (F2, Enter, a double click).
    ** false — editing is impossible: not a table, editing is forbidden, no current row or the column is not editable.
    ** Уже открытая правка не закрывается здесь (вызов идёт из маршрутизатора ввода) — тогда тоже false.
    ** An already open edit is not closed here (the call comes from the input router) — then false too.
    */
    bool beginGridEdit(AppWindow& win, ui::Node& rows, const std::string* typed);

    // ---- grid_edit_editor.cpp ----

    // Редактор столбца: копия <EditTemplate> с полями записи или поле по типу (флажок — для BOOLEAN) / The column editor: an <EditTemplate> copy with the record fields or a field by the type (a check box — for BOOLEAN)
    std::unique_ptr<ui::Node> makeGridEditor(const ui::Node& cell, const ui::GridColumn& column, data::ColumnType type,
        const data::Record& record);

    // Узел значения редактора: первое поле, флажок, переключатель, ползунок или список (nullptr — нет) / The editor value node: the first field, check box, switch, slider or combo box (nullptr — none)
    ui::Node* editorValueNode(ui::Node& editor) noexcept;

    // Сеанс правки без узлов редактора: запись, поле, исходное значение, стек отмены таблицы / An edit session without the editor nodes: the record, the field, the original value, the table undo stack
    std::unique_ptr<GridEdit> makeGridSession(const GridEditPlace& place, const ui::GridColumn& column, data::ColumnType type);

    // ---- grid_edit_value.cpp ----

    // Значение узла редактора текстом (флажок — «true»/«false») / The editor node value as text (a check box — "true"/"false")
    std::string readEditorValue(const ui::Node& value);

    // Поставить значение в узел редактора; поле ввода — выделено целиком (select) или каретка в конце / Put the value into the editor node; an input — selected whole (select) or the caret at the end
    void writeEditorValue(ui::Node& value, const std::string& text, bool select);

    // ---- grid_edit_finish.cpp ----

    /*
    ** Закончить правку: commit — записать значение (через проверку типа) в запись и шаг отмены, иначе отменить.
    ** false — значение не прошло проверку: редактор остаётся открытым с состоянием Invalid.
    **
    ** Finish the edit: commit — write the value (through the type check) into the record and an undo step, otherwise cancel.
    ** false — the value did not pass the check: the editor stays open with the Invalid state.
    */
    bool finishGridEdit(AppWindow& win, bool commit);

    // ---- grid_edit_event.cpp ----

    /*
    ** Спросить код программы событием cellEdit, записывать ли значение text (обработчик может его заменить).
    ** false — отказ: поле значения получает состояние Invalid и подсказку с текстом ошибки, редактор остаётся.
    **
    ** Ask the program code by the cellEdit event whether to write the value text (a handler may replace it).
    ** false — a refusal: the value node gets the Invalid state and a tooltip with the error text, the editor stays.
    */
    bool approveCellEdit(AppWindow& win, std::string& text);

    // Состояние Invalid узла значения редактора: стиль темы показывает ошибку / The Invalid state of the editor value node: the theme style shows the error
    void markGridEditInvalid(ui::Node& value, bool invalid);

    // ---- grid_edit_settle.cpp ----

    // Безопасная точка: отложенное окончание правки, после Tab — правка соседнего столбца; true — дерево изменилось / The safe point: a deferred edit finish, after Tab — editing the neighbor column; true — the tree changed
    bool settleGridEdit(AppWindow& win);

    // Записать открытую правку сейчас (окно закрывают: обработчик onClose должен увидеть значение) / Write the open edit now (the window is closing: the onClose handler must see the value)
    void commitGridEdit(AppWindow& win);

    // ---- grid_edit_watch.cpp ----

    // Поддерево node сейчас будет удалено: ссылки правки на его узлы снимаются / The node subtree is about to be removed: the edit references to its nodes are dropped
    void gridEditRemoving(AppWindow& win, const ui::Node& node) noexcept;

    // Фокус сменился: ушёл из редактора — правка закончится в безопасной точке / The focus changed: it left the editor — the edit finishes at the safe point
    void gridEditFocusChanged(AppWindow& win) noexcept;

    // Узел лежит в поддереве subtree (subtree == nullptr — нет) / The node lies in the subtree (subtree == nullptr — no)
    bool insideNode(const ui::Node* node, const ui::Node* subtree) noexcept;

    // ---- grid_edit_keys.cpp ----

    // Клавиша в редакторе: Enter — записать, Esc — отменить, Tab/Shift+Tab — записать и править соседний столбец (всё — через pending) / A key in the editor: Enter — commit, Esc — cancel, Tab/Shift+Tab — commit and edit the neighbor column (all through pending)
    bool gridEditorKey(AppWindow& win, ui::Node& focus, const platform::Event& event);

    // Двойной щелчок в списке строк: правка ячейки под мышью (её столбец уже текущий) / A double click in the row list: editing the cell under the mouse (its column is already current)
    bool gridDoubleClick(AppWindow& win, ui::Node& rows, const ui::Node& hit);

    // ---- grid_edit_step.cpp ----

    // Шаг отмены записанной правки в стек таблицы (если она ведёт историю) / The undo step of a written edit into the table stack (if it keeps a history)
    void pushCellStep(AppWindow& win, const GridEdit& edit, std::size_t index, const std::string& before,
        const std::string& after);
}

#endif
