#ifndef GRAN_BINDING_INTERNAL_EXPRESSION_BINDING_HPP
#define GRAN_BINDING_INTERNAL_EXPRESSION_BINDING_HPP

#include "gran/binding/expression_text.hpp"
#include "gran/binding/list_bindings.hpp"
#include "gran/data/variable.hpp"

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace beresta
{
    class Document;
    class Element;

    namespace xpath
    {
        class XPathExpression;
        class XPathValue;
        struct XPathContext;
    }
}

namespace gran::binding
{
    // ---- expression_check.cpp ----

    // Текст для компилятора XPath: {name} — поле записи @name / The text for the XPath compiler: {name} is the record field @name
    std::string xpathSource(std::string_view expression);

    /*
    ** Модель в виде XML для запросов XPath: <data><имя_модели><Item поле="…"/>…</имя_модели>…</data>.
    ** Поля записи — атрибуты Item (как в <Data> разметки), поэтому count(files/Item) и @name работают
    ** как обычный XPath над документом. Зеркало — наблюдатель модели: правка модели меняет только свой
    ** элемент Item и номер версии (по нему привязки узнают, что их пора пересчитать).
    **
    ** A model as XML for XPath queries: <data><model_name><Item field="…"/>…</model_name>…</data>.
    ** The record fields are Item attributes (as in the markup <Data>), so count(files/Item) and @name work
    ** as ordinary XPath over a document. The mirror is a model observer: a model edit changes only its own
    ** Item element and the version number (the bindings learn by it that they must be recomputed).
    */
    class MirrorSlot
    {
    public:
        // ---- data_mirror_watch.cpp ----
        MirrorSlot(std::string name, beresta::Element& element);
        virtual ~MirrorSlot();
        MirrorSlot(const MirrorSlot&) = delete;
        MirrorSlot& operator=(const MirrorSlot&) = delete;

        // Список, если элемент отражает список (иначе nullptr) / The list if the element mirrors a list (otherwise nullptr)
        virtual const data::ListSource* list() const noexcept;

        std::string name;
        beresta::Element& element;
        // Начинается с 1: появление модели — тоже изменение для выражения, которое её ждало / Starts at 1: a model appearing is a change too for an expression that waited for it
        std::uint64_t version = 1;
    };

    // Зеркало списка (хранилище, прокси, плоское дерево): дети элемента — только Item по записям / A list mirror (a storage, a proxy, a flat tree): the element children are only Items by the records
    class MirrorEntry final : public MirrorSlot, public data::ListObserver
    {
    public:
        // ---- mirror_entry.cpp ----
        MirrorEntry(std::string name, data::ListSource& model, beresta::Element& element);
        ~MirrorEntry() override;

        void inserted(std::size_t index, std::size_t count) override;
        void removed(std::size_t index, std::size_t count) override;
        void changed(std::size_t index) override;
        const data::ListSource* list() const noexcept override;

        data::ListSource& model;
    };

    /*
    ** Зеркало дерева: вложенные Item повторяют узлы (count(folders//Item) — все узлы). Правки дерева
    ** редки, поэтому элемент строится заново целиком.
    **
    ** A tree mirror: nested Items repeat the nodes (count(folders//Item) — all the nodes). Tree edits
    ** are rare, so the element is rebuilt whole.
    */
    class TreeMirror final : public MirrorSlot, public data::TreeObserver
    {
    public:
        // ---- tree_mirror.cpp ----
        TreeMirror(std::string name, data::TreeModel& tree, beresta::Element& element);
        ~TreeMirror() override;

        // Построить элемент заново (если помечен) / Rebuild the element (if marked)
        void rebuild();

        // ---- tree_mirror_events.cpp ----
        void inserted(data::NodeId parent, std::size_t row, std::size_t count) override;
        void removed(data::NodeId parent, std::size_t row, std::size_t count) override;
        void moved(data::NodeId node, data::NodeId oldParent, std::size_t oldRow) override;
        void changed(data::NodeId node) override;

        data::TreeModel& tree;
        // Дерево менялось с прошлой перестройки (перестройка — перед вычислениями, а не на каждую правку) / The tree changed since the last rebuild (the rebuild is before the evaluations, not on every edit)
        bool dirty = false;
    };

    // Зеркало выделения: копии отмеченных записей (sum(sel/Item/@sum)) / A selection mirror: copies of the marked records (sum(sel/Item/@sum))
    class SelectionMirror final : public MirrorSlot, public data::SelectionObserver
    {
    public:
        // ---- selection_mirror.cpp ----
        SelectionMirror(std::string name, data::SelectionModel& selection, beresta::Element& element);
        ~SelectionMirror() override;
        void selectionChanged() override;

        data::SelectionModel& selection;
    };

    // Документ-зеркало всех моделей; элементы моделей — дети корня <data> / The mirror document of all models; the model elements are children of the <data> root
    class DataMirror
    {
    public:
        // ---- data_mirror.cpp ----
        DataMirror();
        ~DataMirror();
        DataMirror(const DataMirror&) = delete;
        DataMirror& operator=(const DataMirror&) = delete;

        // Начать отражать список (имя не годится для элемента XML — список не отражается) / Start mirroring a list (a name unfit for an XML element — the list is not mirrored)
        void watch(const std::string& name, data::ListSource& model);

        // Корень <data> — узел контекста выражений вне списков / The <data> root is the context node of the expressions outside lists
        const beresta::Element& root() const;

        // ---- data_mirror_versions.cpp ----

        // Элемент записи index модели (nullptr — модель не отражается или записи нет) / The element of the record index of the model (nullptr — the model is not mirrored or there is no record)
        const beresta::Element* record(const data::ListSource& model, std::size_t index) const;

        // Версия модели по имени или по самой модели; 0 — такой модели нет / The model version by name or by the model itself; 0 — there is no such model
        std::uint64_t version(std::string_view name) const noexcept;
        std::uint64_t version(const data::ListSource& model) const noexcept;

        // ---- data_mirror_watch.cpp ----

        // Начать отражать дерево и выделение / Start mirroring a tree and a selection
        void watchTree(const std::string& name, data::TreeModel& tree);
        void watchSelection(const std::string& name, data::SelectionModel& selection);

        // Перестроить изменившиеся деревья (перед проходом вычислений) / Rebuild the changed trees (before an evaluation pass)
        void flush();

    private:
        // Элемент модели под корнем <data>; имя не годится для XML — nullptr / The model element under the <data> root; a name unfit for XML — nullptr
        beresta::Element* createElement(const std::string& name);

        // Документ уничтожается после записей: записи отписываются, ссылаясь на свои элементы / The document is destroyed after the entries: the entries unsubscribe while referring to their elements
        std::unique_ptr<beresta::Document> _document;
        std::vector<std::unique_ptr<MirrorSlot>> _entries;
    };

    // Элемент Item с полями записи как атрибутами (data_mirror.cpp) / An Item element with the record fields as attributes (data_mirror.cpp)
    std::unique_ptr<beresta::Element> mirrorItem(const data::Record& record);

    // Свойство узла, от которого зависит выражение: $self.width, $parent.text, $id.checked / A node property the expression depends on: $self.width, $parent.text, $id.checked
    struct PropertyWatch
    {
        // Имя переменной в выражении ("parent.width"), узел ("self", "parent" или id) и свойство / The variable name in the expression ("parent.width"), the node ("self", "parent" or an id) and the property
        std::string variable;
        std::string base;
        std::string property;
        // Строковое значение при прошлом вычислении / The string value at the previous evaluation
        std::string seen;
    };

    // Источник с номером версии (переменная или модель по имени) и версия при прошлом вычислении / A source with a version number (a variable or a model by name) and the version at the previous evaluation
    struct VersionWatch
    {
        std::string name;
        std::uint64_t seen = 0;
    };

    // Часть значения: литерал или скомпилированное выражение с преобразователем / A part of the value: a literal or a compiled expression with a converter
    struct CompiledPart
    {
        std::string literal;
        std::shared_ptr<const beresta::xpath::XPathExpression> expression;
        std::string converter;
    };

    /*
    ** Куда пишет двусторонняя привязка, когда пользователь меняет поле: в переменную ($имя) или в поле
    ** своей записи ({поле}, @поле). Иное выражение — только одностороннее.
    **
    ** Where a two-way binding writes when the user changes the control: into a variable ($name) or into a field
    ** of its record ({field}, @field). Any other expression is one-way only.
    */
    enum class SourceKind : std::uint8_t
    {
        NONE,
        VARIABLE,
        FIELD
    };

    /*
    ** Привязка одного атрибута узла к выражению. Зависимости собраны при компиляции (переменные,
    ** свойства узлов, имена моделей); пересчёт — только когда хоть одна из них изменилась с прошлого
    ** вычисления, запись в узел — только когда изменился результат.
    **
    ** The binding of one node attribute to an expression. The dependencies are collected at compile time (variables,
    ** node properties, model names); it is recomputed only when at least one of them changed since the previous
    ** evaluation, and written into the node only when the result changed.
    */
    struct ExpressionBinding
    {
        ui::Node* node = nullptr;
        std::string attribute;
        std::vector<CompiledPart> parts;
        std::vector<PropertyWatch> properties;
        std::vector<VersionWatch> variables;
        std::vector<VersionWatch> models;
        // $index — номер записи в списке / $index is the record number in the list
        bool usesIndex = false;
        // Модель записи, к которой относится узел (при прошлом вычислении), и её версия / The record model the node belongs to (at the previous evaluation) and its version
        const data::ListSource* item = nullptr;
        std::uint64_t itemSeen = 0;
        // Выделение списка записи и его версия ($selected, $current) / The record list selection and its version ($selected, $current)
        const data::SelectionModel* selection = nullptr;
        std::uint64_t selectionSeen = 0;
        SourceKind source = SourceKind::NONE;
        std::string sourceName;
        // Значение, записанное в узел последним (для двусторонней — прочитанное из узла после записи) / The value written into the node last (for a two-way one — read from the node after writing)
        std::string applied;
        // stale — ещё не вычислялась (двусторонняя запись не делается); forced — пересчитать, хотя зависимости те же (новый преобразователь)
        // stale — not evaluated yet (no two-way write is made); forced — recompute although the dependencies are the same (a new converter)
        bool stale = true;
        bool forced = false;
    };

    class ExpressionBindings;

    // Отложенная двусторонняя запись: куда и что писать (без ссылки на привязку, которая может уйти) / A deferred two-way write: where and what to write (without a reference to the binding, which may go away)
    struct PendingWrite
    {
        SourceKind source = SourceKind::NONE;
        std::string name;
        std::string value;
        data::VariableType type = data::VariableType::STRING;
        ItemRef item;
    };

    // Всё, что нужно для вычисления: владелец, зеркало, переменные, преобразователи, поиск узлов по id / Everything an evaluation needs: the owner, the mirror, the variables, the converters, the node lookup by id
    struct EvaluationScope
    {
        ListBindings& owner;
        const DataMirror& mirror;
        const ExpressionBindings& bindings;
        // Узлы по (корень окна, id) за один проход update: дерево не меняется во время проверки / The nodes by (window root, id) for one update pass: the tree does not change while checking
        std::map<std::pair<const ui::Node*, std::string>, ui::Node*>& found;
    };

    // ---- expression_compile.cpp ----

    // Привязка атрибута узла; ошибка синтаксиса — GranException(MARKUP) / The binding of a node attribute; a syntax error is GranException(MARKUP)
    std::unique_ptr<ExpressionBinding> compileBinding(ui::Node& node, const ui::AttributeBinding& source);

    // ---- expression_scan.cpp ----

    // Зависимости выражения: $переменные, $узел.свойство, $index и имена (возможные модели) / The expression dependencies: $variables, $node.property, $index and names (possible models)
    void scanDependencies(std::string_view expression, ExpressionBinding& binding);

    // ---- expression_values.cpp ----

    // Узел свойства: self, parent или узел по id в окне привязки (nullptr — нет такого) / The property node: self, parent or a node by id in the binding window (nullptr — none)
    const ui::Node* propertyNode(EvaluationScope& scope, const ExpressionBinding& binding, const std::string& base);

    // Значение свойства узла для XPath: размеры и value ползунка — числа, отметки — логические, прочее — строки / The node property value for XPath: the sizes and the slider value are numbers, the marks are booleans, the rest is strings
    beresta::xpath::XPathValue propertyValue(const ui::Node* node, const std::string& property);

    // Значение переменной для XPath по её типу / The variable value for XPath by its type
    beresta::xpath::XPathValue variableValue(const data::Variable* variable);

    // Значение $база.свойство: свойство выделения (current, count) или узла / The $base.property value: a selection property (current, count) or a node one
    beresta::xpath::XPathValue watchValue(EvaluationScope& scope, const ExpressionBinding& binding, const PropertyWatch& watch);

    // ---- expression_rows.cpp ----

    // Переменные строки списка: $selected, $current, у плоского дерева — $depth, $expanded, $hasChildren / The list row variables: $selected, $current, for a flat tree — $depth, $expanded, $hasChildren
    void rowVariables(const ItemRef& item, beresta::xpath::XPathContext& context);

    // Зарезервированное имя переменной строки / A reserved row variable name
    bool rowVariable(std::string_view name) noexcept;

    /*
    ** Фильтр прокси или плоского дерева выражением XPath 1.0 (паттерн «Стратегия»): узел контекста —
    ** элемент Item записи (как у шаблона списка), переменные — $имя приложения. Когда меняется хоть одна
    ** из его переменных, ExpressionBindings::update просит представление пересчитать фильтр.
    **
    ** A filter of a proxy or a flat tree by an XPath 1.0 expression (the Strategy pattern): the context node is
    ** the record Item element (as for a list template), the variables are the application $names. When at least one
    ** of its variables changes, ExpressionBindings::update asks the view to recompute the filter.
    */
    class ExpressionFilter final : public data::RecordFilter, public data::NodeFilter
    {
    public:
        // ---- expression_filter.cpp ----

        // Ошибка синтаксиса — GranException(INVALID_ARGUMENT) с позицией / A syntax error is GranException(INVALID_ARGUMENT) with the position
        ExpressionFilter(const ExpressionBindings& bindings, const std::string& expression);
        ~ExpressionFilter() override;

        bool accepts(const data::ListSource& source, std::size_t index) const override;
        bool accepts(const data::TreeModel& tree, data::NodeId node) const override;

        // Представление, которое пересчитывается при смене переменных / The view recomputed when the variables change
        void setTarget(data::SortFilterProxy* proxy, data::TreeList* tree) noexcept;

        // ---- expression_filter_watch.cpp ----

        // Сменились ли переменные с прошлой проверки (запоминает новые версии) / Whether the variables changed since the previous check (remembers the new versions)
        bool changed();

        // Пересчитать фильтр представления / Recompute the view filter
        void invalidate();

        // Владелец переменных уходит: фильтр пропускает всё / The variable owner goes away: the filter passes everything
        void detach() noexcept;

    private:
        // Вычислить выражение для записи / Evaluate the expression for a record
        bool evaluate(const data::Record& record) const;

        const ExpressionBindings* _bindings;
        std::shared_ptr<const beresta::xpath::XPathExpression> _expression;
        std::vector<VersionWatch> _variables;
        data::SortFilterProxy* _proxy = nullptr;
        data::TreeList* _tree = nullptr;
    };

    // ---- expression_evaluate.cpp ----

    // Изменилась ли хоть одна зависимость с прошлого вычисления / Whether at least one dependency changed since the previous evaluation
    bool dependenciesChanged(EvaluationScope& scope, ExpressionBinding& binding);

    // Значение привязки текстом (зависимости запоминаются); ошибка вычисления — GranException / The binding value as text (the dependencies are remembered); an evaluation error is GranException
    std::string evaluateBinding(EvaluationScope& scope, ExpressionBinding& binding);

    // ---- expression_target_read.cpp ----

    // Поле содержимого текстового атрибута (text, placeholder, source…); nullptr — атрибут не текстовый / The content field of a text attribute (text, placeholder, source…); nullptr — the attribute is not a text one
    std::string ui::NodeContent::* textMember(const std::string& attribute) noexcept;

    // ---- expression_targets.cpp ----

    // Записать значение в атрибут узла; true — узел изменился / Write a value into the node attribute; true — the node changed
    bool applyTarget(ui::Node& node, const std::string& attribute, const std::string& value);

    // ---- expression_target_read.cpp ----

    // Текущее значение атрибута, который меняет пользователь (текст поля, отметка, value) / The current value of an attribute the user changes (the field text, the mark, value)
    std::string readTarget(const ui::Node& node, const std::string& attribute);

    // Меняет ли пользователь этот атрибут сам (тогда привязка может быть двусторонней) / Whether the user changes this attribute directly (then the binding may be two-way)
    bool editableTarget(const ui::Node& node, const std::string& attribute) noexcept;

    // Логический атрибут (visible, enabled, checked): множество узлов — «есть ли», а не текст первого / A boolean attribute (visible, enabled, checked): a node set means "whether any", not the first one's text
    bool booleanTarget(const std::string& attribute) noexcept;

    /*
    ** Привязки выражений всех окон, переменные, преобразователи и зеркало моделей. Пересчёт
    ** вытягивающий: перед кадром update сверяет зависимости каждой привязки с запомненными
    ** (сравнение строк и номеров версий, без вычисления XPath), и только изменившиеся вычисляются
    ** заново. Так не нужно встраивать уведомления в каждый путь изменения узла (ввод, код,
    ** компоновка), а кадры и так рисуются только по событиям.
    **
    ** The expression bindings of all windows, the variables, the converters and the model mirror. The recomputation
    ** is pull-based: before a frame update compares the dependencies of each binding with the remembered ones
    ** (comparing strings and version numbers, without evaluating XPath), and only the changed ones are evaluated
    ** anew. This way there is no need to build notifications into every path that changes a node (input, code,
    ** layout), and frames are drawn only on events anyway.
    */
    class ExpressionBindings
    {
    public:
        // ---- expression_bindings.cpp ----
        explicit ExpressionBindings(ListBindings& owner);
        ~ExpressionBindings();
        ExpressionBindings(const ExpressionBindings&) = delete;
        ExpressionBindings& operator=(const ExpressionBindings&) = delete;

        // Привязки атрибутов одного узла (обход дерева делает ListBindings::bind) / The attribute bindings of one node (ListBindings::bind walks the tree)
        void bindNode(ui::Node& node);

        // Снять привязки узлов поддерева / Drop the bindings of the subtree nodes
        void forget(const ui::Node& subtree);

        // Снять все привязки (переменные, преобразователи и зеркало остаются) / Drop all bindings (the variables, the converters and the mirror stay)
        void clear() noexcept;

        // ---- expression_update.cpp ----

        // Новая модель: отражать её для XPath / A new model: mirror it for XPath
        void watchModel(const std::string& name, data::ListSource& model);

        // Двусторонние записи, затем пересчёт изменившегося; true — дерево изменилось / The two-way writes, then recomputing what changed; true — the tree changed
        bool update();

        // ---- expression_variables.cpp ----

        // Задать переменную (тип — из variable) / Set a variable (the type is from variable)
        void setVariable(const data::Variable& variable);

        // Задать текстом с прежним типом переменной (новая — строка) / Set by text keeping the former variable type (a new one is a string)
        void setVariableText(const std::string& name, const std::string& text);

        // Переменная по имени (nullptr — нет такой) и версия её последней смены (0 — нет) / A variable by name (nullptr — none) and the version of its last change (0 — none)
        const data::Variable* variable(std::string_view name) const noexcept;
        std::uint64_t variableVersion(std::string_view name) const noexcept;

        // ---- expression_convert.cpp ----

        // Зарегистрировать преобразователь; привязки пересчитываются при следующем update / Register a converter; the bindings are recomputed at the next update
        void setConverter(const std::string& name, Converter converter);

        // Значение через toView преобразователя (нет такого — как есть) / The value through the converter toView (none — as it is)
        std::string toView(const std::string& converter, const std::string& value) const;

        // Обратное преобразование; false — у преобразователя нет toSource / The back conversion; false — the converter has no toSource
        bool toSource(const std::string& converter, const std::string& value, std::string& result) const;

        // ---- expression_filters.cpp ----

        // Отражать дерево и выделение для XPath / Mirror a tree and a selection for XPath
        void watchTree(const std::string& name, data::TreeModel& tree);
        void watchSelection(const std::string& name, data::SelectionModel& selection);

        // Новый фильтр выражением; его пересчёт при смене переменных — забота update / A new expression filter; its recomputation when the variables change is the job of update
        std::shared_ptr<ExpressionFilter> makeFilter(const std::string& expression);

    private:
        // ---- expression_filters.cpp ----

        // Пересчитать фильтры, чьи переменные изменились / Recompute the filters whose variables changed
        void refreshFilters();

        // ---- expression_update.cpp ----

        // Вычислить и записать в узел; true — узел изменился / Evaluate and write into the node; true — the node changed
        bool refresh(EvaluationScope& scope, ExpressionBinding& binding);

        // Пользователь изменил поле — запомнить запись в источник / The user changed the control — remember a write into the source
        void collectWrite(ExpressionBinding& binding, std::vector<PendingWrite>& writes);

        // Записать значение в переменную или поле записи / Write the value into a variable or a record field
        void applyWrite(const PendingWrite& write);

        struct VariableSlot
        {
            data::Variable value;
            std::uint64_t version = 0;
        };

        ListBindings& _owner;
        // Зеркало объявлено до привязок: привязки читают его элементы / The mirror is declared before the bindings: the bindings read its elements
        DataMirror _mirror;
        std::map<std::string, VariableSlot, std::less<>> _variables;
        std::map<std::string, Converter, std::less<>> _converters;
        std::vector<std::unique_ptr<ExpressionBinding>> _bindings;
        // Фильтры живут в представлениях; здесь — слабые ссылки для пересчёта / The filters live in the views; here are weak references for the recomputation
        std::vector<std::weak_ptr<ExpressionFilter>> _filters;
        // Счётчик версий переменных (общий: новая переменная не совпадёт с прежней версией) / The variable version counter (shared: a new variable does not match a former version)
        std::uint64_t _clock = 0;
    };
}

#endif
