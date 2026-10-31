#include "gran/markup/internal/build_context.hpp"

#include "beresta.hpp"

namespace gran::markup
{
    /*
    ** Пробельные отступы между тегами парсер не хранит (без них дерево меньше, а Text с участками
    ** собирается как прежде), поэтому вывод форматируется заново: два пробела на уровень, как в
    ** samples/. Смешанное содержимое (Text с Span, Button с текстом) сериализатор пишет в строку,
    ** не добавляя пробелов, — перечитанный файл даёт тот же интерфейс. Комментарии сохраняются.
    ** Документ без DOM (собран не загрузчиком) — пустая строка.
    **
    ** The parser does not keep the whitespace indentation between tags (the tree is smaller without it, and a Text with runs
    ** is assembled as before), so the output is formatted anew: two spaces per level, as in
    ** samples/. The serializer writes mixed content (a Text with Spans, a Button with text) inline
    ** without adding spaces — the file read again gives the same interface. Comments are kept.
    ** A document without a DOM (not built by the loader) is an empty string.
    */
    std::string serializeMarkup(const UiDocument& document)
    {
        beresta::SerializerOptions options;

        if (!document.dom)
            return std::string();
        options.prettyPrint = true;
        options.indent = "  ";
        return beresta::XmlSerializer(options).toString(*document.dom);
    }
}
