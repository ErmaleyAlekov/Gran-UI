#include "gran/markup/internal/components.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

#include "beresta.hpp"

#include <vector>

namespace gran::markup::internal
{
    namespace
    {
        // Документ схемы с позициями (ошибки схемы укажут строку) / A schema document with positions (the schema errors point at the line)
        beresta::Document parseSchemaText(std::string_view text)
        {
            beresta::ParserOptions options;

            options.trackLocations = true;
            return beresta::XmlParser(options).parse(text);
        }

        // Встроенная схема как документ — разбирается один раз и только читается / The embedded schema as a document — parsed once and only read
        const beresta::Document& embeddedSchemaDocument()
        {
            static const beresta::Document DOCUMENT = parseSchemaText(std::string_view(
                reinterpret_cast<const char*>(generated::GRAN_UI_SCHEMA), generated::GRAN_UI_SCHEMA_SIZE));

            return DOCUMENT;
        }
    }

    /*
    ** Встроенная схема компилируется заново вместе со схемами библиотек: сгенерированные ссылаются на её
    ** типы (UiElement, Color…), а скомпилированная схема «Берёсты» неизменяема и не дополняется.
    **
    ** The embedded schema is compiled anew together with the library schemas: the generated ones refer to its
    ** types (UiElement, Color…), while a compiled Beresta schema is immutable and cannot be extended.
    */
    void compileComponentSchema(ComponentSet& set)
    {
        std::vector<beresta::Document> generated;
        std::vector<const beresta::Document*> documents{&embeddedSchemaDocument()};
        std::size_t library = 0;

        generated.reserve(set.libraries.size());
        try
        {
            for (library = 0; library < set.libraries.size(); ++library)
            {
                set.libraries[library].schemaText = librarySchemaText(set, library);
                generated.push_back(parseSchemaText(set.libraries[library].schemaText));
                documents.push_back(&generated.back());
            }
            // Ошибку общей сборки приписываем последней библиотеке / An error of the common build is attributed to the last library
            library = set.libraries.size() - 1;
            set.schema = beresta::xsd::Schema::fromDocuments(documents);
        }
        catch (const std::exception& error)
        {
            throw MarkupException(ErrorCode::MARKUP, i18n::format(i18n::Message::MARKUP_LIBRARY_SCHEMA,
                {set.libraries[library].path, error.what()}), 0, 0);
        }
    }

    // Как у разбора: позиция первой ошибки, текст — все сообщения / As for parsing: the first error position, the text is all messages
    void checkSchema(const beresta::Document& dom, const std::shared_ptr<const beresta::xsd::Schema>& schema)
    {
        std::string text;
        const beresta::Diagnostic* first = nullptr;
        std::vector<beresta::Diagnostic> diagnostics = beresta::xsd::SchemaValidator(schema).validate(dom);

        for (const beresta::Diagnostic& diagnostic : diagnostics)
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

    std::shared_ptr<const beresta::xsd::Schema> documentSchema(const UiDocument& document)
    {
        if (document.components && document.components->schema)
            return document.components->schema;
        return markupSchema();
    }
}
