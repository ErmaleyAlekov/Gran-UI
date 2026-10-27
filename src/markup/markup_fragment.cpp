#include "gran/markup/internal/build_context.hpp"
#include "gran/core/gran_exception.hpp"

#include "beresta.hpp"

#include <string>

namespace gran::markup::internal
{
    namespace
    {
        /*
        ** Фрагмент проверяется как полный документ: <Window> с заглушками объявленных стилей
        ** (иначе ссылки style="…" не пройдут keyref схемы) и самим фрагментом со второй строки.
        ** Имена стилей — NCName (их проверила схема основного файла), экранировать нечего.
        ** Обёртка не даёт фрагменту ничего лишнего: второй <Resources>, второй корень или DOCTYPE
        ** внутри элемента — ошибки разбора или схемы.
        **
        ** The fragment is checked as a full document: <Window> with stubs of the declared styles
        ** (otherwise style="…" references fail the schema keyref) and the fragment itself from the second line.
        ** The style names are NCName (the schema of the main file has checked them), nothing to escape.
        ** The wrapper gives the fragment nothing extra: a second <Resources>, a second root or a DOCTYPE
        ** inside an element are parse or schema errors.
        */
        std::string wrapFragment(std::string_view text, const UiDocument& document)
        {
            std::vector<std::string_view> names = document.styles.names();
            std::string wrapped = "<Window>";

            if (!names.empty())
            {
                wrapped += "<Resources>";
                for (std::string_view name : names)
                    wrapped.append("<Style name=\"").append(name).append("\"/>");
                wrapped += "</Resources>";
            }
            wrapped.append("\n").append(text).append("\n</Window>");
            return wrapped;
        }

        // Номер строки обёртки → номер строки фрагмента (строка 1 — сама обёртка) / A wrapper line number → a fragment line number (line 1 is the wrapper itself)
        std::size_t fragmentLine(std::size_t line) noexcept
        {
            if (line > 1)
                return line - 1;
            return 1;
        }

        // Разбор обёртки; позиция ошибки пересчитывается к фрагменту / Parsing the wrapper; the error position is converted to the fragment
        beresta::Document parseFragment(const std::string& wrapped)
        {
            try
            {
                return parseMarkupText(wrapped);
            }
            catch (const MarkupException& error)
            {
                throw MarkupException(error.getCode(), error.getMessage(), fragmentLine(error.getLine()),
                    error.getColumn());
            }
        }

        // Позиции узлов — тоже относительно фрагмента / The node positions are relative to the fragment too
        void shiftLines(ui::Node& node) noexcept
        {
            node.line = fragmentLine(node.line);
            if (node.itemTemplate)
                shiftLines(*node.itemTemplate);
            for (const std::unique_ptr<ui::Node>& child : node.children)
                shiftLines(*child);
        }
    }
}

namespace gran::markup
{
    // Построение тем же buildNode, что и у основного файла, со стилями и темой документа
    // Building with the same buildNode as the main file, with the document styles and theme
    std::unique_ptr<ui::Node> buildFragment(std::string_view text, UiDocument& document)
    {
        beresta::Document dom = internal::parseFragment(internal::wrapFragment(text, document));
        LoadOptions options;
        options.baseDirectory = document.baseDirectory;
        internal::BuildContext context{options, document, document.tokens, 0};
        const beresta::Element* root = internal::firstElementChild(*dom.getRoot());

        if (root->getLocalName() == "Resources")
            root = internal::nextElementSibling(*root);
        std::unique_ptr<ui::Node> node = internal::buildNode(*root, context);
        internal::shiftLines(*node);
        document.nodeCount += context.nodeCount;
        return node;
    }
}
