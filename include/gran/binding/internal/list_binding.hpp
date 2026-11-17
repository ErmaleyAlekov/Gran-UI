#ifndef GRAN_BINDING_INTERNAL_LIST_BINDING_HPP
#define GRAN_BINDING_INTERNAL_LIST_BINDING_HPP

#include "gran/binding/internal/extent_index.hpp"
#include "gran/binding/list_bindings.hpp"
#include "gran/data/selection_model.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace gran::binding
{
    // Одно поле узла, зависящее от записи: куда писать и шаблон текста / One node field depending on the record: where to write and the text template
    struct FieldBinding
    {
        ui::Node* node;
        std::string ui::NodeContent::* member;
        std::string pattern;
    };

    // Узлы одной записи: корень копии шаблона и его поля с подстановками / The nodes of one record: the template copy root and its fields with placeholders
    struct BoundItem
    {
        ui::Node* root;
        std::vector<FieldBinding> fields;
    };

    /*
    ** Привязка контейнера items="модель" к модели (паттерн Observer, наблюдатель — эта привязка).
    ** На каждую запись — копия <ItemTemplate> с подставленными полями; правка модели меняет только
    ** затронутые копии: вставка и удаление — свои узлы, смена поля — только тексты с подстановками
    ** (фокус, выделение и прокрутка остальных записей сохраняются). Копии идут после собственных
    ** детей контейнера, в порядке записей. Живёт в ListBindings, пока контейнер в дереве.
    **
    ** The binding of an items="model" container to the model (the Observer pattern, this binding is the observer).
    ** For every record — a copy of <ItemTemplate> with the fields substituted; a model edit changes only
    ** the affected copies: an insertion and a removal — their own nodes, a field change — only the texts with placeholders
    ** (the focus, the selection and the scrolling of the other records survive). The copies follow the container's
    ** own children, in the record order. It lives in ListBindings while the container is in the tree.
    **
    ** Виртуальный список (virtual="true"): копии есть только у «окна» записей [first, first + k) около
    ** видимой области прокрутки, остальные записи заменяют две распорки (Box) с суммой размеров своих записей
    ** (ExtentIndex: измеренные — точно, неизмеренные — средним). После каждой компоновки updateWindow измеряет
    ** копии окна и подгоняет окно под видимую область, поэтому список из 100 000 записей разной высоты стоит
    ** как список из нескольких десятков.
    **
    ** A virtual list (virtual="true"): only the "window" of records [first, first + k) near the visible scroll area
    ** has copies, the other records are replaced by two spacers (Box) sized by the sum of their records
    ** (ExtentIndex: the measured ones exactly, the unmeasured ones by the average). After every layout updateWindow measures
    ** the window copies and fits the window to the visible area, so a list of 100,000 records of different heights
    ** costs as much as a list of a few dozen.
    **
    ** С моделью выделения (selection="…") копии отмеченных записей получают состояние стиля Selected;
    ** перестановка записей (сортировка прокси) переставляет готовые копии, а не строит их заново.
    ** Виртуальный список, дошедший до конца записей, просит у источника следующую порцию (fetchMore).
    **
    ** With a selection model (selection="…") the copies of the marked records get the Selected style state;
    ** a permutation of the records (a proxy sort) rearranges the ready copies instead of building them anew.
    ** A virtual list that reached the end of the records asks the source for the next portion (fetchMore).
    */
    class ListBinding final : public data::ListObserver, public data::SelectionObserver
    {
    public:
        // ---- list_binding.cpp ----

        // Подписка и копии для уже имеющихся записей; selection может быть nullptr / The subscription and copies for the records already present; selection may be nullptr
        ListBinding(ListBindings& owner, ui::Node& container, data::ListSource& model, data::SelectionModel* selection);
        ~ListBinding() override;
        ListBinding(const ListBinding&) = delete;
        ListBinding& operator=(const ListBinding&) = delete;

        void inserted(std::size_t index, std::size_t count) override;
        void removed(std::size_t index, std::size_t count) override;
        void changed(std::size_t index) override;

        // ---- list_binding_order.cpp ----

        void moved(std::size_t from, std::size_t count, std::size_t to) override;
        void reordered(const std::vector<std::size_t>& oldToNew) override;
        void selectionChanged() override;

        // ---- list_binding_window.cpp ----

        // Подогнать окно виртуального списка под видимую область; true — дерево изменилось (нужна перекомпоновка)
        // Fit the virtual list window to the visible area; true — the tree changed (a relayout is needed)
        bool updateWindow();

        // ---- list_binding_query.cpp ----

        // Привязанный контейнер / The bound container
        const ui::Node& container() const noexcept;

        // Модель привязки / The binding model
        data::ListSource& model() const noexcept;

        // Модель выделения (nullptr — нет) / The selection model (nullptr — none)
        data::SelectionModel* selection() const noexcept;

        // Номер записи, чья копия — узел item (прямой ребёнок контейнера); size_t(-1) — не копия / The record index whose copy is the item node (a direct container child); size_t(-1) — not a copy
        std::size_t indexOf(const ui::Node& item) const noexcept;

        // Копия записи index (nullptr — запись вне окна виртуального списка) / The copy of the record index (nullptr — the record is outside the virtual list window)
        ui::Node* itemAt(std::size_t index) const noexcept;

        // ---- list_binding_reveal.cpp ----

        // Прокрутить ближайший Scroll так, чтобы запись index была видна (по вертикали) / Scroll the nearest Scroll so that the record index is visible (vertically)
        void reveal(std::size_t index);

    private:
        // ---- list_binding_order.cpp ----

        // Переставить копии обычного списка по перестановке записей / Rearrange the copies of an ordinary list by the record permutation
        void permute(const std::vector<std::size_t>& oldToNew);

        // Состояние Selected копии по модели выделения / The Selected state of a copy by the selection model
        void markSelection(const BoundItem& item, std::size_t record);

        // ---- list_binding_items.cpp ----

        // Место в детях контейнера для копии с номером local в окне / The place among the container children for the copy with the window index local
        std::size_t childPosition(std::size_t local) const noexcept;

        // Копия записи record (record — внутри окна или сразу за ним) / A copy of the record (record is inside the window or right after it)
        void materialize(std::size_t record);

        // Убрать копию с номером local в окне / Remove the copy with the window index local
        void dematerialize(std::size_t local);

        // Распорки до и после окна (только виртуальный список) / The spacers before and after the window (a virtual list only)
        void createSpacers();

        // ---- list_binding_spacers.cpp ----

        // Размеры распорок по размерам записей вне окна; true — размер изменился / The spacer sizes by the sizes of the records outside the window; true — a size changed
        bool applySpacers();

        // Следующая порция источника, когда окно дошло до конца; true — записи добавились / The next source portion when the window reached the end; true — records were added
        bool fetchMore();

        // ---- list_binding_window.cpp ----

        // Окно стало [first, last): лишние копии уходят, недостающие создаются / The window became [first, last): extra copies go, missing ones are created
        bool reshapeWindow(std::size_t first, std::size_t last);

        // ---- list_binding_measure.cpp ----

        /*
        ** Записать в индекс размеры копий окна по их рамкам (размер плюс промежуток стека).
        ** Вызывается после компоновки: все копии окна к этому моменту скомпонованы.
        **
        ** Write the sizes of the window copies into the index by their frames (the size plus the stack gap).
        ** It is called after a layout: all window copies are laid out by this moment.
        */
        void measureItems(bool horizontal);

        /*
        ** Якорь прокрутки (как overflow-anchor браузеров). Запись у верха видимой области запоминается вместе
        ** со своим местом по индексу размеров (_anchorAt). После измерения место по индексу меняется (оценка
        ** сменилась точным размером, сменилось среднее) — Scroll сдвигается на разницу, и видимые записи
        ** остаются на месте экрана, а окно и прокрутка считаются в одних координатах. Якорь не виден (прыжок
        ** полосой прокрутки, правка модели) — сдвиг берётся по первой видимой копии: от её места в компоновке
        ** до места по индексу, куда её поставят новые распорки. Прокрутка пользователя место по индексу
        ** не меняет и поэтому не гасится. start — начало содержимого контейнера, [from, to) — видимая
        ** область, всё в координатах окна.
        **
        ** The scroll anchor (like overflow-anchor in browsers). The record at the top of the visible area is remembered together
        ** with its place by the size index (_anchorAt). After a measurement the place by the index changes (an estimate was
        ** replaced by an exact size, the average changed) — the Scroll moves by the difference, and the visible records
        ** stay in their place on the screen, while the window and the scroll are computed in the same coordinates. The anchor
        ** is not visible (a jump with the scroll bar, a model edit) — the shift is taken by the first visible copy: from its place
        ** in the layout to its place by the index, where the new spacers will put it. The user's scrolling does not change
        ** the place by the index and therefore is not cancelled. start is the container content start, [from, to) is
        ** the visible area, all in window coordinates.
        */
        float anchorShift(float start, float from, float to, bool horizontal) const;

        // Прокрутить ближайший Scroll на shift по главной оси; true — прокрутка сдвинута (нужна перекомпоновка) / Scroll the nearest Scroll by shift along the main axis; true — the scroll moved (a relayout is needed)
        bool anchorScroll(float shift, bool horizontal);

        ListBindings& _owner;
        ui::Node& _container;
        data::ListSource& _model;
        data::SelectionModel* _selection;
        // Копии окна записей [_first, _first + _items.size()); у обычного списка окно — все записи
        // The copies of the record window [_first, _first + _items.size()); for an ordinary list the window is all records
        std::vector<BoundItem> _items;
        std::size_t _first = 0;
        bool _virtual = false;
        // Сколько копий можно держать без компоновки (до первого кадра — начальная порция) / How many copies may be kept without a layout (before the first frame — the initial portion)
        std::size_t _capacity = 32;
        // Размеры записей по главной оси вместе с промежутком (только виртуальный список) / The record sizes along the main axis with the gap (a virtual list only)
        ExtentIndex _extents;
        // Якорь прокрутки: номер записи (SIZE_MAX — нет) и её место по индексу размеров / The scroll anchor: the record index (SIZE_MAX — none) and its place by the size index
        std::size_t _anchor = static_cast<std::size_t>(-1);
        double _anchorAt = 0.0;
        // Источник отказал при подгрузке — больше не просить / The source failed while fetching — do not ask any more
        bool _fetchFailed = false;
        ui::Node* _lead = nullptr;
        ui::Node* _trail = nullptr;
    };

    // ---- list_binding_fields.cpp ----

    // Поля с подстановками {поле} во всём поддереве копии (вложенные шаблоны не входят) / The fields with {field} placeholders in the whole copy subtree (nested templates are not included)
    std::vector<FieldBinding> collectFields(ui::Node& root);

    // Подставить поля записи; изменившиеся узлы помечаются к перекомпоновке / Substitute the record fields; the changed nodes are marked for relayout
    void applyFields(const std::vector<FieldBinding>& fields, const data::Record& record);

    // ---- list_bindings_grid.cpp: список строк таблицы DataGrid / the row list of a DataGrid table ----

    // Клавиша текущего столбца (влево, вправо, Home, End); false — не таблица или клавиша не про столбцы / A current column key (left, right, Home, End); false — not a table or the key is not about columns
    bool gridKey(ListBinding& binding, ListKey key, bool control);

    // Нажатие в строку row: столбец ячейки под узлом hit становится текущим / A press in the row: the column of the cell under the hit node becomes current
    void gridPress(ListBinding& binding, const ui::Node& row, const ui::Node& hit);

    // Стрелка сортировки в заголовке по ключам прокси, на который смотрит таблица (после привязки) / The header sort arrow by the keys of the proxy the table looks at (after binding)
    void syncGridSort(const ui::Node& rows, const data::ListSource& model);
}

#endif
