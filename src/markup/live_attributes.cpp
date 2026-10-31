#include "gran/markup/internal/build_context.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

#include "beresta.hpp"

#include <utility>

namespace gran::markup::internal
{
    namespace
    {
        // Запись или удаление атрибута; недопустимое имя XML — понятная ошибка вместо исключения «Берёсты»
        // Writing or removing an attribute; an invalid XML name is a clear error instead of a Beresta exception
        void writeAttribute(beresta::Element& element, std::string_view name, const std::optional<std::string>& value)
        {
            try
            {
                if (value)
                    element.setAttribute(name, *value);
                else
                    element.removeAttribute(name);
            }
            catch (const beresta::DomException& error)
            {
                throw GranException(ErrorCode::INVALID_ARGUMENT,
                    i18n::format(i18n::Message::MARKUP_BAD_ATTRIBUTE, {name, error.what()}));
            }
        }

        /*
        ** Проверка всего DOM той же схемой, что и при загрузке: правка одного атрибута может нарушить
        ** правила уровня документа (уникальность id, ссылки style, xs:assert). Ошибки — как у разбора:
        ** позиция первой, текст — все сообщения.
        **
        ** Checking the whole DOM by the same schema as at load time: editing one attribute may break
        ** document level rules (id uniqueness, style references, xs:assert). The errors are as for parsing:
        ** the position of the first one, the text is all messages.
        */
        void checkSchema(const beresta::Document& dom)
        {
            std::string text;
            const beresta::Diagnostic* first = nullptr;

            for (const beresta::Diagnostic& diagnostic : beresta::xsd::SchemaValidator(markupSchema()).validate(dom))
            {
                if (diagnostic.severity == beresta::ErrorSeverity::WARNING)
                    continue;
                if (first == nullptr)
                    first = &diagnostic;
                else
                    text += '\n';
                text += diagnostic.message;
            }
            if (first != nullptr)
                throw MarkupException(ErrorCode::MARKUP, text, first->position.line, first->position.column);
        }
    }
}

namespace gran::markup
{
    // Чтение прямо из DOM: там и то, что записал код, и то, что было в файле / Reading right from the DOM: it holds both what the code wrote and what was in the file
    std::optional<std::string> markupAttribute(const ui::Node& node, std::string_view name)
    {
        if (node.domElement == nullptr)
            return std::nullopt;
        return node.domElement->getAttribute(name);
    }

    /*
    ** Порядок «записать → проверить → построить» с откатом: и ошибка схемы, и ошибка значения,
    ** которую ловит только построитель (например, неизвестный $токен), возвращают атрибуты как были.
    **
    ** The order "write → check → build" with a rollback: both a schema error and a value error
    ** that only the builder catches (for example, an unknown $token) bring the attributes back as they were.
    */
    std::unique_ptr<ui::Node> editMarkupAttribute(UiDocument& document, const ui::Node& node, std::string_view name,
        const std::optional<std::string>& value)
    {
        beresta::Element* element = internal::editableElement(node);

        if (element == nullptr || !document.dom || element->getOwnerDocument() != document.dom.get())
            throw GranException(ErrorCode::INVALID_ARGUMENT,
                i18n::format(i18n::Message::MARKUP_NOT_LIVE, {ui::elementName(node.kind)}));
        std::vector<beresta::Attribute> previous = element->getAttributes();
        internal::writeAttribute(*element, name, value);
        try
        {
            internal::checkSchema(*document.dom);
            return rebuildFromElement(document, *element);
        }
        catch (...)
        {
            element->setAttributes(std::move(previous));
            throw;
        }
    }

    // Контекст как у фрагментов: стили, тема и каталог картинок документа / The context as for fragments: the document styles, theme and image directory
    std::unique_ptr<ui::Node> rebuildFromElement(UiDocument& document, const beresta::Element& element)
    {
        LoadOptions options;

        options.baseDirectory = document.baseDirectory;
        internal::BuildContext context{options, document, document.tokens, 0};
        return internal::buildNode(element, context);
    }
}
