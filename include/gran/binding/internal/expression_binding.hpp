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
    class MirrorEntry final : public data::ListObserver
    {
    public:
        // ---- mirror_entry.cpp ----
        MirrorEntry(std::string name, data::ListModel& model, beresta::Element& element);
        ~MirrorEntry() override;
        MirrorEntry(const MirrorEntry&) = delete;
        MirrorEntry& operator=(const MirrorEntry&) = delete;

        void inserted(std::size_t index, std::size_t count) override;
        void removed(std::size_t index, std::size_t count) override;
        void changed(std::size_t index) override;

        std::string name;
        data::ListModel& model;
        beresta::Element& element;
        // Начинается с 1: появление модели — тоже изменение для выражения, которое её ждало / Starts at 1: a model appearing is a change too for an expression that waited for it
        std::uint64_t version = 1;
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

        // Начать отражать модель (имя не годится для элемента XML — модель не отражается) / Start mirroring a model (a name unfit for an XML element — the model is not mirrored)
        void watch(const std::string& name, data::ListModel& model);

        // Корень <data> — узел контекста выражений вне списков / The <data> root is the context node of the expressions outside lists
        const beresta::Element& root() const;

        // ---- data_mirror_versions.cpp ----

        // Элемент записи index модели (nullptr — модель не отражается или записи нет) / The element of the record index of the model (nullptr — the model is not mirrored or there is no record)
        const beresta::Element* record(const data::ListModel& model, std::size_t index) const;

        // Версия модели по имени или по самой модели; 0 — такой модели нет / The model version by name or by the model itself; 0 — there is no such model
        std::uint64_t version(std::string_view name) const noexcept;
        std::uint64_t version(const data::ListModel& model) const noexcept;

    private:
        // Документ уничтожается после записей: записи отписываются, ссылаясь на свои элементы / The document is destroyed after the entries: the entries unsubscribe while referring to their elements
        std::unique_ptr<beresta::Document> _document;
        std::vector<std::unique_ptr<MirrorEntry>> _entries;
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
        const data::ListModel* item = nullptr;
        std::uint64_t itemSeen = 0;
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
        void watchModel(const std::string& name, data::ListModel& model);

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

    private:
        // ---- expression_update.cpp ----

        // Вычислить и записать в узел; true — узел изменился / Evaluate and write into the node; true — the node changed
        bool refresh(EvaluationScope& scope, ExpressionBinding& binding);

        // Пользователь изменил поле — значение уходит в источник / The user changed the control — the value goes into the source
        void writeBack(ExpressionBinding& binding);

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
        // Счётчик версий переменных (общий: новая переменная не совпадёт с прежней версией) / The variable version counter (shared: a new variable does not match a former version)
        std::uint64_t _clock = 0;
    };
}

#endif
