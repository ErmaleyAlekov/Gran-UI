#ifndef GRAN_MARKUP_INTERNAL_COMPONENTS_HPP
#define GRAN_MARKUP_INTERNAL_COMPONENTS_HPP

#include "gran/markup/internal/build_context.hpp"

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace beresta::xsd
{
    class Schema;
}

namespace gran::markup
{
    // Тип параметра <Param type> / The <Param type> parameter type
    enum class ParamType : std::uint8_t { STRING, NUMBER, BOOLEAN, COLOR, PAINT, LENGTH, SIZE, ACTION, MODEL };

    // Параметр компонента / A component parameter
    struct ComponentParam
    {
        std::string name;
        ParamType type = ParamType::STRING;
        bool required = false;
        std::optional<std::string> defaultValue;
        // Перечисление values (пусто — любое значение типа) / The values enumeration (empty — any value of the type)
        std::vector<std::string> values;
    };

    // Имя стиля в теле → имя, под которым стиль зарегистрирован в наборе документа / A style name in the body → the name the style is registered under in the document set
    using StyleNames = std::map<std::string, std::string, std::less<>>;

    /*
    ** Объявление <Component>: элементы живут в DOM библиотеки (ComponentLibrary::dom), поэтому
    ** указатели действительны, пока жив набор компонентов документа.
    **
    ** A <Component> declaration: the elements live in the library DOM (ComponentLibrary::dom), so
    ** the pointers are valid while the document component set is alive.
    */
    struct ComponentSpec
    {
        std::string name;
        std::size_t library = 0;
        const beresta::Element* body = nullptr;
        std::vector<ComponentParam> params;
        // Имена именованных слотов и есть ли безымянный / The named slot names and whether there is an unnamed one
        std::vector<std::string> slots;
        bool defaultSlot = false;
        // Стили компонента и библиотеки (свои закрывают библиотечные) / The component and library styles (the own ones hide the library ones)
        StyleNames styles;
    };

    // Подключённая библиотека: файл, пространство имён, DOM и сгенерированная схема / An included library: the file, the namespace, the DOM and the generated schema
    struct ComponentLibrary
    {
        std::string path;
        std::string namespaceUri;
        std::shared_ptr<beresta::Document> dom;
        std::string schemaText;
    };

    /*
    ** Компоненты документа: библиотеки, объявления и схема «gran-ui.xsd + схемы библиотек», которой
    ** проверяется документ при загрузке и при каждой живой правке.
    **
    ** The document components: the libraries, the declarations and the schema "gran-ui.xsd + the library schemas" that
    ** checks the document at load time and on every live edit.
    */
    struct ComponentSet
    {
        std::vector<ComponentLibrary> libraries;
        std::vector<ComponentSpec> components;
        std::shared_ptr<const beresta::xsd::Schema> schema;
    };
}

namespace gran::markup::internal
{
    /*
    ** Развёртка, которая сейчас строится: компонент, экземпляр и развёртка, в теле которой экземпляр
    ** написан (outer). Содержимое слотов строится в области outer — там, где его написали.
    **
    ** The expansion being built now: the component, the instance and the expansion in whose body the instance
    ** is written (outer). The slot content is built in the outer scope — where it was written.
    */
    struct ComponentScope
    {
        const ComponentSpec* spec = nullptr;
        const beresta::Element* instance = nullptr;
        const ComponentScope* outer = nullptr;
    };

    /*
    ** Подстановки одной развёртки: значения заданных параметров (атрибут экземпляра или default),
    ** локальные id тела → полные («id экземпляра.id»), тема и набор стилей документа.
    **
    ** The substitutions of one expansion: the values of the set parameters (an instance attribute or default),
    ** the local body ids → the full ones ("instance id.id"), the theme and the document style set.
    */
    struct Substitution
    {
        const ComponentSpec* spec = nullptr;
        std::map<std::string, std::string, std::less<>> values;
        std::map<std::string, std::string, std::less<>> ids;
        // Приставка локальных имён: id экземпляра или «Компонент-строка-столбец» / The local name prefix: the instance id or "Component-line-column"
        std::string prefix;
        const TokenMap* tokens = nullptr;
        const style::StyleSheet* styles = nullptr;
    };

    // ---- component_library.cpp ----

    // <xi:include> в Resources: библиотеки, их схема и проверка документа ею / <xi:include> in Resources: the libraries, their schema and the document check by it
    void loadComponents(const beresta::Element& resources, BuildContext& context);


    // ---- component_read.cpp ----

    // <Library>: стили библиотеки и компонентов в набор документа, объявления компонентов / <Library>: the library and component styles into the document set, the component declarations
    void readLibrary(ComponentSet& set, std::size_t library, BuildContext& context);

    // ---- component_params.cpp ----

    // <Param> в описание; имя, занятое атрибутом экземпляра, — ошибка / A <Param> into a description; a name taken by an instance attribute is an error
    ComponentParam readParam(const beresta::Element& param, const std::string& component);

    // Атрибуты экземпляра, которые получает корень тела (id, visible, компоновка) / The instance attributes the body root gets (id, visible, the layout)
    bool isHostAttribute(std::string_view name) noexcept;

    // Имя стиля тела → зарегистрированное (компонент → библиотека → окно); нет нигде — GranException / A body style name → the registered one (component → library → window); nowhere — GranException
    std::string resolveStyleName(std::string_view name, const StyleNames& names, const style::StyleSheet& styles);

    // ---- component_schema.cpp ----

    // Текст XSD библиотеки: элемент на компонент, параметры — атрибуты / The library XSD text: an element per component, the parameters are attributes
    std::string librarySchemaText(const ComponentSet& set, std::size_t library);

    // ---- component_validate.cpp ----

    // Схема «gran-ui.xsd + схемы библиотек» / The schema "gran-ui.xsd + the library schemas"
    void compileComponentSchema(ComponentSet& set);

    // Ошибки схемы (без предупреждений) → MarkupException: позиция первой, текст — все / Schema errors (without warnings) → MarkupException: the first position, the text is all
    void checkSchema(const beresta::Document& dom, const std::shared_ptr<const beresta::xsd::Schema>& schema);

    // Схема, которой проверяется документ: с библиотеками или встроенная / The schema the document is checked by: with the libraries or the embedded one
    std::shared_ptr<const beresta::xsd::Schema> documentSchema(const UiDocument& document);

    // ---- component_public.cpp ----

    // Библиотеки обоих документов одинаковы (тогда частичная перезагрузка возможна) / The libraries of both documents are the same (then a partial reload is possible)
    bool sameComponents(const UiDocument& current, const UiDocument& next);

    // ---- component_expand.cpp ----

    // Узел из экземпляра: копия тела с подстановками, корень ссылается на экземпляр / A node from an instance: the body copy with substitutions, the root refers to the instance
    std::unique_ptr<ui::Node> expandComponent(const beresta::Element& instance, BuildContext& context);

    // ---- component_slots.cpp ----

    // Слоты тела (и тел вложенных экземпляров — пересылка слотов) в описание / The body slots (and of the nested instance bodies — slot forwarding) into the description
    void collectSlots(const beresta::Element& element, ComponentSpec& spec);

    // Дети экземпляра идут в существующие слоты; иначе — MarkupException / The instance children go into existing slots; otherwise MarkupException
    void checkSlots(const beresta::Element& instance, const ComponentSpec& spec);

    // <Slot> тела: содержимое экземпляра (в области outer) или запасное / A body <Slot>: the instance content (in the outer scope) or the fallback
    void buildSlot(const beresta::Element& slot, ui::Node& node, BuildContext& context);

    // ---- component_scope.cpp ----

    // Значения параметров и карта локальных id для экземпляра / The parameter values and the local id map for an instance
    Substitution makeSubstitution(const beresta::Element& instance, const ComponentSpec& spec, const BuildContext& context);

    // ---- component_shadow.cpp ----

    // Копия тела с подставленными параметрами, id, стилями и атрибутами хозяина / The body copy with the parameters, ids, styles and host attributes substituted
    std::shared_ptr<beresta::Document> makeShadow(const beresta::Element& instance, const Substitution& substitution);

    // ---- component_text.cpp ----

    // Атрибут целиком {$p} у незаданного параметра → nullopt (атрибута нет); иначе текст с подстановками / A whole {$p} attribute of an unset parameter → nullopt (no attribute); otherwise the text with substitutions
    std::optional<std::string> substituteAttribute(std::string_view value, const Substitution& substitution);

    // Текст с выражениями {…}: параметры и локальные id подставлены / A text with {…} expressions: the parameters and local ids substituted
    std::string substituteText(std::string_view value, const Substitution& substitution);

    // ---- component_values.cpp ----

    // Параметр, на который целиком ссылается выражение ({$имя}, пробелы вокруг допустимы), или nullptr / The parameter a whole expression refers to ({$name}, spaces around are allowed) or nullptr
    const ComponentParam* wholeParameter(std::string_view expression, const Substitution& substitution) noexcept;

        // Значение параметра внутри выражения XPath: строка в кавычках, число, true()/false() или (выражение) / A parameter value inside an XPath expression: a quoted string, a number, true()/false() or (an expression)
    std::string parameterExpression(const ComponentParam& param, const Substitution& substitution);
}

#endif
