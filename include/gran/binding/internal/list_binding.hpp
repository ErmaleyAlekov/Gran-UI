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

        // ---- list_binding_query.cpp ----

        // Привязанный контейнер / The bound container
        const ui::Node& container() const noexcept;

        // Модель привязки / The binding model
        data::ListModel& model() const noexcept;

        // Номер записи, чья копия — узел item (прямой ребёнок контейнера); size_t(-1) — не копия / The record index whose copy is the item node (a direct container child); size_t(-1) — not a copy
        std::size_t indexOf(const ui::Node& item) const noexcept;

    private:
        // Место в детях контейнера для копии записи index / The place among the container children for the copy of the record index
        std::size_t childPosition(std::size_t index) const noexcept;

        ListBindings& _owner;
        ui::Node& _container;
        data::ListModel& _model;
        std::vector<BoundItem> _items;
    };

    // ---- list_binding_fields.cpp ----

    // Поля с подстановками {поле} во всём поддереве копии (вложенные шаблоны не входят) / The fields with {field} placeholders in the whole copy subtree (nested templates are not included)
    std::vector<FieldBinding> collectFields(ui::Node& root);

    // Подставить поля записи; изменившиеся узлы помечаются к перекомпоновке / Substitute the record fields; the changed nodes are marked for relayout
    void applyFields(const std::vector<FieldBinding>& fields, const data::Record& record);
}

#endif
