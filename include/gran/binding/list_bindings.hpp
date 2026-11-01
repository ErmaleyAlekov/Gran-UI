#ifndef GRAN_BINDING_LIST_BINDINGS_HPP
#define GRAN_BINDING_LIST_BINDINGS_HPP

#include "gran/binding/expression_text.hpp"
#include "gran/data/list_model.hpp"
#include "gran/data/selection_model.hpp"
#include "gran/data/sort_filter_proxy.hpp"
#include "gran/data/tree_list.hpp"
#include "gran/data/tree_model.hpp"
#include "gran/data/variable.hpp"
#include "gran/markup/markup_loader.hpp"
#include "gran/ui/node.hpp"

#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace gran::binding
{
    class ListBinding;
    class ExpressionBindings;

    /*
    ** Запись списка, к которой относится узел; model == nullptr — узел не из списка. model — источник
    ** списка (хранилище, прокси или плоское дерево), selection — его модель выделения (может не быть).
    **
    ** The list record a node belongs to; model == nullptr — the node is not from a list. model is the list
    ** source (a storage, a proxy or a flat tree), selection is its selection model (may be absent).
    */
    struct ItemRef
    {
        data::ListSource* model = nullptr;
        std::size_t index = 0;
        data::SelectionModel* selection = nullptr;
    };

    // Клавиша навигации по списку с выделением (без зависимости от платформы) / A navigation key for a list with a selection (without a platform dependency)
    enum class ListKey : std::uint8_t { UP, DOWN, HOME, END, PAGE_UP, PAGE_DOWN, LEFT, RIGHT, SPACE, SELECT_ALL };

    /*
    ** Слушатель удаления узлов: тот, кто хранит невладеющие ссылки на узлы (ввод, анимации,
    ** доступность), снимает их до уничтожения поддерева. Вызывается без исключений наружу.
    **
    ** A node removal listener: whoever keeps non-owning references to nodes (input, animations,
    ** accessibility) drops them before the subtree is destroyed. It is called without exceptions escaping.
    */
    class RemovalListener
    {
    public:
        virtual ~RemovalListener() = default;

        // Поддерево node сейчас будет уничтожено / The node subtree is about to be destroyed
        virtual void nodeRemoving(const ui::Node& node) noexcept = 0;
    };

    /*
    ** Модели списков по именам и привязки контейнеров items="…" к ним. Не зависит от окна
    ** и GPU: тем же классом пользуются Application (с слушателем удаления) и рендер вне экрана
    ** (gran_snapshot строит списки из <Data> без окна). Порядок полей: привязки уничтожаются
    ** раньше моделей, от которых отписываются.
    **
    ** List models by name and the bindings of items="…" containers to them. It does not depend on the window
    ** and the GPU: the same class serves Application (with a removal listener) and offscreen rendering
    ** (gran_snapshot builds lists from <Data> without a window). The field order: the bindings are destroyed
    ** before the models they unsubscribe from.
    */
    class ListBindings
    {
    public:
        // ---- list_bindings.cpp ----

        // listener может быть nullptr (некому снимать ссылки) / listener may be nullptr (nobody to drop references)
        explicit ListBindings(RemovalListener* listener = nullptr);
        ~ListBindings();
        ListBindings(const ListBindings&) = delete;
        ListBindings& operator=(const ListBindings&) = delete;

        // Привязки для всех контейнеров items="…" и атрибутов {…} в поддереве (после загрузки или вставки) / Bindings for all items="…" containers and {…} attributes in the subtree (after loading or inserting)
        void bind(ui::Node& subtree);

        // Снять все привязки (модели и переменные остаются) — перед заменой документа / Drop all bindings (the models and the variables stay) — before replacing the document
        void clear() noexcept;

        // ---- list_bindings_tree.cpp ----

        // Снять привязки контейнеров, лежащих в поддереве / Drop the bindings of the containers lying in the subtree
        void forget(const ui::Node& subtree);

        /*
        ** Удалить узел с поддеревом: привязки внутри снимаются, слушатель получает nodeRemoving,
        ** узел уходит из детей родителя, родитель помечается к перекомпоновке. Корень (без
        ** родителя) не удаляется.
        **
        ** Remove a node with its subtree: the bindings inside are dropped, the listener gets nodeRemoving,
        ** the node leaves the parent children, the parent is marked for relayout. The root (without
        ** a parent) is not removed.
        */
        void removeNode(ui::Node& node);

        // Запись ближайшего списка, из копии шаблона которой узел / The record of the nearest list whose template copy holds the node
        ItemRef itemOf(const ui::Node& node) const noexcept;

        // ---- list_bindings_window.cpp ----

        /*
        ** После компоновки: окна виртуальных списков подгоняются под видимую область прокрутки.
        ** true — дерево изменилось: нужны проход стилей и компоновка ещё раз (обычно хватает одного).
        **
        ** After a layout: the virtual list windows are fitted to the visible scroll area.
        ** true — the tree changed: a style pass and a layout are needed once more (usually one is enough).
        */
        bool updateVirtual();

        /*
        ** Пересчитать выражения {…}, чьи зависимости изменились (переменные, модели, свойства узлов,
        ** размеры после компоновки), и записать значения полей, изменённых пользователем, в источники
        ** двусторонних привязок. Вызывается перед стилями кадра и после компоновки. true — дерево
        ** изменилось: нужны проход стилей и компоновка ещё раз.
        **
        ** Recompute the {…} expressions whose dependencies changed (variables, models, node properties,
        ** sizes after a layout), and write the values of the controls changed by the user into the sources
        ** of the two-way bindings. It is called before the frame styles and after a layout. true — the tree
        ** changed: a style pass and a layout are needed once more.
        */
        bool updateExpressions();

        // ---- list_bindings_models.cpp ----

        // Хранилище списка по имени; нет — создаётся пустое; имя другого вида — GranException / A list storage by name; none — an empty one is created; a name of another kind — GranException
        data::ListModel& model(const std::string& name);

        // Дерево по имени; нет — создаётся пустое / A tree by name; none — an empty one is created
        data::TreeModel& tree(const std::string& name);

        // Любой список по имени: хранилище, прокси или плоское дерево (nullptr — нет) / Any list by name: a storage, a proxy or a flat tree (nullptr — none)
        data::ListSource* list(std::string_view name) const noexcept;

        // Модель выделения по имени (nullptr — нет) / A selection model by name (nullptr — none)
        data::SelectionModel* selection(std::string_view name) const noexcept;

        // Имя списка (пусто — список без имени) / The list name (empty — a list without a name)
        std::string nameOf(const data::ListSource& source) const;

        // ---- list_bindings_views.cpp ----

        // Прокси сортировки и фильтра над списком source / A sort and filter proxy over the source list
        data::SortFilterProxy& createProxy(const std::string& name, const std::string& source);

        // Плоское представление дерева tree / A flat view of the tree
        data::TreeList& createTreeList(const std::string& name, const std::string& tree);

        // Модель выделения над списком source / A selection model over the source list
        data::SelectionModel& createSelection(const std::string& name, const std::string& source,
            data::SelectionMode mode);

        // ---- list_bindings_lookup.cpp ----

        // Фильтр прокси или плоского дерева выражением XPath (пусто — без фильтра) / A filter of a proxy or a flat tree by an XPath expression (empty — no filter)
        void setFilterExpression(const std::string& view, const std::string& expression);

        // Прокси и плоское дерево по имени (nullptr — нет такого или другой вид) / A proxy and a flat tree by name (nullptr — none or another kind)
        data::SortFilterProxy* proxy(std::string_view name) const noexcept;
        data::TreeList* treeList(std::string_view name) const noexcept;

        // ---- list_bindings_select.cpp ----

        /*
        ** Нажатие в контейнере с выделением: запись под узлом hit становится текущей, отметки — по правилам
        ** режима (Ctrl, Shift). false — контейнер без выделения или нажатие мимо записей.
        **
        ** A press in a container with a selection: the record under the hit node becomes current, the marks follow the
        ** mode rules (Ctrl, Shift). false — a container without a selection or a press outside the records.
        */
        bool pressItem(const ui::Node& container, const ui::Node& hit, bool control, bool shift);

        /*
        ** Клавиша у контейнера с выделением: стрелки, Home/End, PageUp/PageDown двигают текущую запись (Shift —
        ** диапазон, Ctrl — без смены отметок), пробел отмечает, Ctrl+A — все; у плоского дерева стрелки
        ** влево/вправо сворачивают и раскрывают. Текущая запись прокручивается в видимую область.
        **
        ** A key at a container with a selection: the arrows, Home/End, PageUp/PageDown move the current record (Shift —
        ** a range, Ctrl — without changing the marks), space marks, Ctrl+A — all; for a flat tree the left/right
        ** arrows collapse and expand. The current record is scrolled into the visible area.
        */
        bool keyItem(const ui::Node& container, ListKey key, bool control, bool shift);

        // ---- list_bindings_adopt.cpp ----

        // Данные Resources: хранилища и деревья заменяют содержимое, представления создаются или перенастраиваются / The Resources data: the storages and the trees replace the contents, the views are created or reconfigured
        void adoptData(std::vector<markup::DataSet>& sets);

        // ---- list_bindings_variables.cpp ----

        // Задать переменную привязки $имя (тип — из variable) / Set the $name binding variable (the type is from variable)
        void setVariable(const data::Variable& variable);

        // Задать переменную текстом, сохранив её тип (новая — строка) / Set a variable by text keeping its type (a new one is a string)
        void setVariableText(const std::string& name, const std::string& text);

        // Переменная по имени; nullptr — нет такой / A variable by name; nullptr — none
        const data::Variable* variable(std::string_view name) const noexcept;

        // Преобразователь значений для выражений «{выражение ; имя}» / A value converter for the "{expression ; name}" expressions
        void setConverter(const std::string& name, Converter converter);

        // Переменные <Variable> разметки задают значения (как <Data> — содержимое моделей) / The markup <Variable> variables set the values (as <Data> sets the model contents)
        void adoptVariables(std::vector<data::Variable>& variables);

    private:
        // Представление по имени: прокси или плоское дерево / A view by name: a proxy or a flat tree
        struct View
        {
            std::string name;
            std::unique_ptr<data::ListSource> source;
        };

        // ---- list_bindings_lookup.cpp ----

        // Привязка контейнера (nullptr — контейнер не привязан) / The container binding (nullptr — the container is not bound)
        ListBinding* bindingOf(const ui::Node& container) const noexcept;

        // ---- list_bindings_views.cpp ----

        // Имя свободно или занято моделью вида kind; иначе — GranException(INVALID_ARGUMENT) / The name is free or taken by a model of kind; otherwise — GranException(INVALID_ARGUMENT)
        void checkName(const std::string& name, const char* kind) const;

        // ---- list_bindings_adopt.cpp ----

        // Представление из разметки: создать или перенастроить существующее / A view from the markup: create or reconfigure an existing one
        void adoptView(const markup::DataSet& set);

        /*
        ** Порядок полей — порядок зависимостей: представления опираются на хранилища, выделения — на
        ** представления, зеркало выражений и привязки — на всех. Деструктор снимает их в обратном порядке
        ** явно: представления уничтожаются от последнего к первому (прокси над прокси).
        **
        ** The field order is the dependency order: the views rest on the storages, the selections — on
        ** the views, the expression mirror and the bindings — on all of them. The destructor drops them in the reverse order
        ** explicitly: the views are destroyed from the last to the first (a proxy over a proxy).
        */
        RemovalListener* _listener;
        std::map<std::string, std::unique_ptr<data::ListModel>, std::less<>> _models;
        std::map<std::string, std::unique_ptr<data::TreeModel>, std::less<>> _trees;
        std::vector<View> _views;
        std::map<std::string, std::unique_ptr<data::SelectionModel>, std::less<>> _selections;
        std::unique_ptr<ExpressionBindings> _expressions;
        std::vector<std::unique_ptr<ListBinding>> _bindings;
    };
}

#endif
