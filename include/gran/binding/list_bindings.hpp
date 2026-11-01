#ifndef GRAN_BINDING_LIST_BINDINGS_HPP
#define GRAN_BINDING_LIST_BINDINGS_HPP

#include "gran/binding/expression_text.hpp"
#include "gran/data/list_model.hpp"
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

    // Запись списка, к которой относится узел; model == nullptr — узел не из списка
    // The list record a node belongs to; model == nullptr — the node is not from a list
    struct ItemRef
    {
        data::ListModel* model = nullptr;
        std::size_t index = 0;
    };

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

        // Модель по имени; нет — создаётся пустая / A model by name; none — an empty one is created
        data::ListModel& model(const std::string& name);

        // Привязки для всех контейнеров items="…" и атрибутов {…} в поддереве (после загрузки или вставки) / Bindings for all items="…" containers and {…} attributes in the subtree (after loading or inserting)
        void bind(ui::Node& subtree);

        // Снять все привязки (модели и переменные остаются) — перед заменой документа / Drop all bindings (the models and the variables stay) — before replacing the document
        void clear() noexcept;

        // ---- list_bindings_tree.cpp ----

        // Снять привязки контейнеров, лежащих в поддереве / Drop the bindings of the containers lying in the subtree
        void forget(const ui::Node& subtree);

        // Данные <Data> разметки заменяют содержимое моделей / The markup <Data> data replaces the model contents
        void adoptData(std::vector<markup::DataSet>& sets);

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
        RemovalListener* _listener;
        std::map<std::string, std::unique_ptr<data::ListModel>, std::less<>> _models;
        // После моделей: зеркало выражений отписывается от них раньше, чем они уходят / After the models: the expression mirror unsubscribes from them before they go
        std::unique_ptr<ExpressionBindings> _expressions;
        std::vector<std::unique_ptr<ListBinding>> _bindings;
    };
}

#endif
