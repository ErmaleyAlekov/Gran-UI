#include "gran/markup/internal/build_context.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"
#include "gran/res/resources.hpp"

#include "beresta.hpp"

namespace gran::markup::internal
{
    namespace
    {
        // Разбор со схемой и всеми ошибками сразу / Parsing with the schema and all errors at once
        beresta::ParserOptions parserOptions(beresta::CollectingErrorHandler& handler)
        {
            beresta::ParserOptions options;

            options.schema = markupSchema();
            options.trackLocations = true;
            options.errorHandler = &handler;
            options.maxDepth = 256;
            return options;
        }

        /*
        ** Ошибки схемы: позиция — у первой, текст — все сообщения (каждое уже содержит
        ** строку, столбец и путь вида /Window/Grid/Button[2]).
        **
        ** Schema errors: the position is the first one's, the text is all messages (each already holds
        ** the line, the column and a path like /Window/Grid/Button[2]).
        */
        void throwIfInvalid(const beresta::CollectingErrorHandler& handler)
        {
            const std::vector<beresta::Diagnostic>& diagnostics = handler.getDiagnostics();
            std::string text;

            if (!handler.hasErrors())
                return;
            for (const beresta::Diagnostic& diagnostic : diagnostics)
            {
                if (!text.empty())
                    text += '\n';
                text += diagnostic.message;
            }
            throw MarkupException(ErrorCode::MARKUP, text, diagnostics.front().position.line,
                diagnostics.front().position.column);
        }
    }

    // Текст: синтаксическая ошибка XML (фатальная) → MarkupException
    // A text: a fatal XML syntax error → MarkupException
    beresta::Document parseMarkupText(std::string_view text)
    {
        beresta::CollectingErrorHandler handler;
        beresta::Document dom;

        try
        {
            dom = beresta::XmlParser(parserOptions(handler)).parse(text);
        }
        catch (const beresta::ParseException& error)
        {
            throw MarkupException(ErrorCode::MARKUP, error.getMessage(), error.getLine(), error.getColumn());
        }
        throwIfInvalid(handler);
        return dom;
    }

    // Файл: вдобавок ошибка чтения → GranException(IO) / A file: in addition a read error → GranException(IO)
    beresta::Document parseMarkupFile(const std::string& path)
    {
        beresta::CollectingErrorHandler handler;
        beresta::Document dom;
        res::ResourceView view;

        // Ресурс res:/ разбирается из памяти программы / A res:/ resource is parsed from the program memory
        if (res::find(path, view))
            return parseMarkupText(std::string_view(reinterpret_cast<const char*>(view.data), view.size));
        try
        {
            dom = beresta::XmlParser(parserOptions(handler)).parseFile(path);
        }
        catch (const beresta::ParseException& error)
        {
            throw MarkupException(ErrorCode::MARKUP, error.getMessage(), error.getLine(), error.getColumn());
        }
        catch (const beresta::DomException&)
        {
            throw GranException(ErrorCode::IO, i18n::format(i18n::Message::IO_READ_FAILED, {path}));
        }
        throwIfInvalid(handler);
        return dom;
    }
}
