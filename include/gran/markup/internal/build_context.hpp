#ifndef GRAN_MARKUP_INTERNAL_BUILD_CONTEXT_HPP
#define GRAN_MARKUP_INTERNAL_BUILD_CONTEXT_HPP

#include "gran/markup/markup_loader.hpp"

#include <map>
#include <string>
#include <string_view>

namespace beresta
{
    class Element;
    class Document;
    struct Attribute;
}

namespace gran::generated
{
    // Встроенный текст схемы (gen/markup/gran_ui_schema.cpp) / The embedded schema text (gen/markup/gran_ui_schema.cpp)
    extern const unsigned char GRAN_UI_SCHEMA[];
    extern const std::size_t GRAN_UI_SCHEMA_SIZE;
}

namespace gran::markup::internal
{
    // Состояние построения одного документа / The build state of one document
    struct BuildContext
    {
        const LoadOptions& options;
        UiDocument& document;
        TokenMap tokens;
        std::size_t nodeCount = 0;
    };

    // ---- markup_parse.cpp ----

    // Разбор текста «Берёстой» со схемой; ошибки → MarkupException / Parsing a text by Beresta with the schema; errors → MarkupException
    beresta::Document parseMarkupText(std::string_view text);

    // Разбор файла; ошибка чтения → GranException(IO) / Parsing a file; a read error → GranException(IO)
    beresta::Document parseMarkupFile(const std::string& path);

    // ---- document_builder.cpp ----

    // Дерево UI из проверенного схемой DOM / The UI tree from a schema-checked DOM
    UiDocument buildDocument(const beresta::Document& dom, const LoadOptions& options);

    // ---- style_resources.cpp ----

    // <Resources>: тема, шрифты, стили / <Resources>: the theme, fonts, styles
    void readResources(const beresta::Element& resources, BuildContext& context);

    // ---- keyframes_resources.cpp ----

    // <Keyframes name><Frame at …/>…</Keyframes> в набор стилей документа / <Keyframes name><Frame at …/>…</Keyframes> into the document style set
    void readKeyframes(const beresta::Element& keyframes, BuildContext& context);

    // Все имена анимаций значения animation объявлены в <Keyframes>; иначе GranException(RESOURCE)
    // All animation names of an animation value are declared in <Keyframes>; otherwise GranException(RESOURCE)
    void checkAnimationNames(const style::PropertyValue& value, const BuildContext& context);

    // ---- declarations.cpp ----

    // Один атрибут-свойство в набор; false — это не свойство стиля
    // One property attribute into a set; false — it is not a style property
    bool readDeclaration(const beresta::Element& element, const beresta::Attribute& attribute,
        style::PropertySet& set, BuildContext& context);

    // Атрибуты-свойства элемента в набор (остальные пропускаются) / The property attributes of an element into a set (others are skipped)
    void readDeclarations(const beresta::Element& element, style::PropertySet& set, BuildContext& context);

    // Дочерние <Hover>, <Pressed>... в объявления состояний / The <Hover>, <Pressed>... children into state declarations
    void readStates(const beresta::Element& element, style::DeclarationBlock& block, BuildContext& context);

    // Текст атрибута с подставленными $значениями темы / The attribute text with $theme values substituted
    std::string substituteTokens(std::string_view value, const BuildContext& context);

    // Ошибка значения → MarkupException с позицией элемента / A value error → MarkupException with the element position
    [[noreturn]] void throwAttributeError(const beresta::Element& element, std::string_view attribute,
        const std::string& message);

    // ---- data_resources.cpp ----

    // <Data name><Item …/>…</Data>: записи модели списка; уникальность имён проверила схема / <Data name><Item …/>…</Data>: the list model records; the schema has checked the name uniqueness
    void readDataSet(const beresta::Element& data, BuildContext& context);

    // ---- dom_helpers.cpp ----

    // Первый дочерний элемент или nullptr / The first child element or nullptr
    const beresta::Element* firstElementChild(const beresta::Element& element) noexcept;

    // Следующий соседний элемент или nullptr / The next sibling element or nullptr
    const beresta::Element* nextElementSibling(const beresta::Element& element) noexcept;

    // Значение атрибута или пустая строка / The attribute value or an empty string
    const std::string* findAttribute(const beresta::Element& element, std::string_view name) noexcept;

    // ---- node_builder.cpp ----

    // Узел UI и его поддерево / A UI node and its subtree
    std::unique_ptr<ui::Node> buildNode(const beresta::Element& element, BuildContext& context);

    // ---- node_children.cpp ----

    // Вид элемента по имени тега; false — это не элемент интерфейса (а состояние или шаблон) / The element kind by the tag name; false — it is not a UI element (but a state or a template)
    bool elementKind(std::string_view name, ui::ElementKind& kind) noexcept;

    // Дети-элементы узла и шаблон записи списка (<ItemTemplate>) / The element children of a node and the list record template (<ItemTemplate>)
    void buildChildren(const beresta::Element& element, ui::Node& node, BuildContext& context);

    // ---- node_content.cpp ----

    // Атрибут содержимого (text, source, onClick...); false — это не атрибут содержимого
    // A content attribute (text, source, onClick...); false — it is not a content attribute
    bool readContentAttribute(ui::Node& node, std::string_view name, const std::string& value);

    // ---- icon_shape.cpp ----

    // Фигура <Icon data viewBox fillRule>; ошибка пути или viewBox — MarkupException с позицией элемента
    // The <Icon data viewBox fillRule> shape; a path or viewBox error is MarkupException with the element position
    void readIconShape(const beresta::Element& element, ui::Node& node);

    // ---- node_text.cpp ----

    // Текст тега по виду элемента; true — Text построен участками (его дети уже есть) / The tag text by the element kind; true — the Text is built from runs (its children exist already)
    bool readTagText(const beresta::Element& element, ui::Node& node, BuildContext& context);

    // Текст тега TextArea как есть, без первого перевода строки после открывающего тега (как у <textarea>)
    // The TextArea tag text as is, without the first line break after the opening tag (as for <textarea>)
    std::string fieldText(const std::string& text);

    // ---- text_runs_builder.cpp ----

    // Есть ли среди детей элемента Span (тогда Text строится участками) / Whether there is a Span among the element children (then the Text is built from runs)
    bool hasSpanElements(const beresta::Element& element) noexcept;

    // Участки Text по порядку: текст между Span — анонимные участки; пробелы схлопываются через границы
    // The Text runs in order: the text between Spans is anonymous runs; spaces collapse across the boundaries
    void buildRuns(const beresta::Element& element, ui::Node& text, BuildContext& context);
}

#endif
