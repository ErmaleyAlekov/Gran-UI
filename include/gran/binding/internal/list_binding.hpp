#ifndef GRAN_BINDING_INTERNAL_LIST_BINDING_HPP
#define GRAN_BINDING_INTERNAL_LIST_BINDING_HPP

#include "gran/binding/list_bindings.hpp"

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
    ** видимой области прокрутки, остальные записи заменяют две распорки (Box) с размером «число записей ×
    ** средний размер записи». После каждой компоновки updateWindow подгоняет окно под видимую область,
    ** поэтому список из 100 000 записей стоит как список из нескольких десятков.
    **
    ** A virtual list (virtual="true"): only the "window" of records [first, first + k) near the visible scroll area
    ** has copies, the other records are replaced by two spacers (Box) sized "the record count × the average
    ** record size". After every layout updateWindow fits the window to the visible area,
    ** so a list of 100,000 records costs as much as a list of a few dozen.
    */
    class ListBinding final : public data::ListObserver
    {
    public:
        // ---- list_binding.cpp ----

        // Подписка и копии для уже имеющихся записей / The subscription and copies for the records already present
        ListBinding(ListBindings& owner, ui::Node& container, data::ListModel& model);
        ~ListBinding() override;
        ListBinding(const ListBinding&) = delete;
        ListBinding& operator=(const ListBinding&) = delete;

        void inserted(std::size_t index, std::size_t count) override;
        void removed(std::size_t index, std::size_t count) override;
        void changed(std::size_t index) override;

        // ---- list_binding_window.cpp ----

        // Подогнать окно виртуального списка под видимую область; true — дерево изменилось (нужна перекомпоновка)
        // Fit the virtual list window to the visible area; true — the tree changed (a relayout is needed)
        bool updateWindow();

        // ---- list_binding_query.cpp ----

        // Привязанный контейнер / The bound container
        const ui::Node& container() const noexcept;

        // Модель привязки / The binding model
        data::ListModel& model() const noexcept;

        // Номер записи, чья копия — узел item (прямой ребёнок контейнера); size_t(-1) — не копия / The record index whose copy is the item node (a direct container child); size_t(-1) — not a copy
        std::size_t indexOf(const ui::Node& item) const noexcept;

    private:
        // Место в детях контейнера для копии с номером local в окне / The place among the container children for the copy with the window index local
        std::size_t childPosition(std::size_t local) const noexcept;

        // ---- list_binding_items.cpp ----

        // Копия записи record (record — внутри окна или сразу за ним) / A copy of the record (record is inside the window or right after it)
        void materialize(std::size_t record);

        // Убрать копию с номером local в окне / Remove the copy with the window index local
        void dematerialize(std::size_t local);

        // Распорки до и после окна (только виртуальный список) / The spacers before and after the window (a virtual list only)
        void createSpacers();

        // ---- list_binding_window.cpp ----

        // Размеры распорок по числу записей вне окна; true — размер изменился / The spacer sizes by the record count outside the window; true — a size changed
        bool applySpacers();

        // Окно стало [first, last): лишние копии уходят, недостающие создаются / The window became [first, last): extra copies go, missing ones are created
        bool reshapeWindow(std::size_t first, std::size_t last);

        ListBindings& _owner;
        ui::Node& _container;
        data::ListModel& _model;
        // Копии окна записей [_first, _first + _items.size()); у обычного списка окно — все записи
        // The copies of the record window [_first, _first + _items.size()); for an ordinary list the window is all records
        std::vector<BoundItem> _items;
        std::size_t _first = 0;
        bool _virtual = false;
        // Сколько копий можно держать без компоновки (до первого кадра — начальная порция) / How many copies may be kept without a layout (before the first frame — the initial portion)
        std::size_t _capacity = 32;
        // Средний размер записи по главной оси вместе с промежутком (0 — ещё не измерен) / The average record size along the main axis with the gap (0 — not measured yet)
        float _extent = 0.0f;
        ui::Node* _lead = nullptr;
        ui::Node* _trail = nullptr;
    };

    // ---- list_binding_fields.cpp ----

    // Поля с подстановками {поле} во всём поддереве копии (вложенные шаблоны не входят) / The fields with {field} placeholders in the whole copy subtree (nested templates are not included)
    std::vector<FieldBinding> collectFields(ui::Node& root);

    // Подставить поля записи; изменившиеся узлы помечаются к перекомпоновке / Substitute the record fields; the changed nodes are marked for relayout
    void applyFields(const std::vector<FieldBinding>& fields, const data::Record& record);
}

#endif
