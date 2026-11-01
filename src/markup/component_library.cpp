#include "gran/markup/internal/components.hpp"
#include "gran/core/gran_exception.hpp"
#include "gran/core/i18n.hpp"

#include "beresta.hpp"

#include <filesystem>
#include <utility>

namespace gran::markup::internal
{
    namespace
    {
        // Пространство имён XInclude 1.0 / The XInclude 1.0 namespace
        constexpr std::string_view XINCLUDE_NAMESPACE = "http://www.w3.org/2001/XInclude";

        // Ошибка внутри файла библиотеки: позиция — её, к тексту добавляется путь / An error inside a library file: the position is its own, the path is added to the text
        [[noreturn]] void rethrowInLibrary(const MarkupException& error, const std::string& path)
        {
            throw MarkupException(error.getCode(), i18n::format(i18n::Message::MARKUP_IN_LIBRARY, {path, error.getMessage()}),
                error.getLine(), error.getColumn());
        }

        /*
        ** Путь файла из href: относительный — от каталога разметки (как у картинок). Включение
        ** всегда целым файлом-библиотекой: parse="text" и xpointer здесь смысла не имеют.
        **
        ** The file path from href: a relative one is from the markup directory (as for images). The inclusion
        ** is always a whole library file: parse="text" and xpointer make no sense here.
        */
        std::string includePath(const beresta::Element& include, const BuildContext& context)
        {
            std::optional<std::string> href = include.getAttribute("href");
            std::optional<std::string> parse = include.getAttribute("parse");

            if (include.getLocalName() != "include" || !href.has_value() || href->empty()
                || (parse.has_value() && *parse != "xml") || include.hasAttribute("xpointer"))
                throwElementError(include, i18n::format(i18n::Message::MARKUP_BAD_INCLUDE));
            std::filesystem::path path(*href);
            if (path.is_relative() && !context.options.baseDirectory.empty())
                path = std::filesystem::path(context.options.baseDirectory) / path;
            return path.lexically_normal().string();
        }

        /*
        ** Файл разбирается той же встроенной схемой (её глобальный элемент <Library>); экземпляры
        ** в телах проверит потом схема с библиотеками. Пространство имён — одно на библиотеку.
        **
        ** The file is parsed with the same embedded schema (its global element <Library>); the instances
        ** in the bodies are checked later by the schema with the libraries. One namespace per library.
        */
        void loadLibrary(const beresta::Element& include, ComponentSet& set, BuildContext& context)
        {
            ComponentLibrary library;

            library.path = includePath(include, context);
            try
            {
                library.dom = std::make_shared<beresta::Document>(parseMarkupFile(library.path));
            }
            catch (const MarkupException& error)
            {
                rethrowInLibrary(error, library.path);
            }
            const beresta::Element& root = *library.dom->getRoot();
            if (root.getLocalName() != "Library" || isComponentInstance(root))
                throwElementError(include, i18n::format(i18n::Message::MARKUP_NOT_LIBRARY, {library.path, root.getName()}));
            library.namespaceUri = *findAttribute(root, "namespace");
            for (const ComponentLibrary& other : set.libraries)
            {
                if (other.namespaceUri == library.namespaceUri)
                    throwElementError(include, i18n::format(i18n::Message::MARKUP_DUPLICATE_LIBRARY, {library.namespaceUri}));
            }
            set.libraries.push_back(std::move(library));
            readLibrary(set, set.libraries.size() - 1, context);
        }

        // Ошибки разбора объявлений — с путём библиотеки / The declaration reading errors — with the library path
        void loadChecked(const beresta::Element& include, ComponentSet& set, BuildContext& context)
        {
            std::size_t count = set.libraries.size();

            try
            {
                loadLibrary(include, set, context);
            }
            catch (const MarkupException& error)
            {
                if (set.libraries.size() == count)
                    throw;
                rethrowInLibrary(error, set.libraries.back().path);
            }
        }
    }

    /*
    ** Библиотеки читаются после стилей окна (стили компонентов могут опираться на них). Затем
    ** окно и каждая библиотека проверяются общей схемой: так экземпляры и в окне, и в телах
    ** других компонентов получают проверку параметров и содержимого.
    **
    ** The libraries are read after the window styles (the component styles may be based on them). Then
    ** the window and every library are checked by the common schema: this way the instances both in the window and in the bodies
    ** of other components get their parameters and content checked.
    */
    void loadComponents(const beresta::Element& resources, BuildContext& context)
    {
        std::shared_ptr<ComponentSet> set = std::make_shared<ComponentSet>();

        for (const beresta::Element* child = firstElementChild(resources); child != nullptr; child = nextElementSibling(*child))
        {
            if (child->getNamespaceURI() == std::optional<std::string>(XINCLUDE_NAMESPACE))
                loadChecked(*child, *set, context);
        }
        if (set->libraries.empty())
            return;
        compileComponentSchema(*set);
        checkSchema(*resources.getOwnerDocument(), set->schema);
        for (const ComponentLibrary& library : set->libraries)
        {
            try
            {
                checkSchema(*library.dom, set->schema);
            }
            catch (const MarkupException& error)
            {
                rethrowInLibrary(error, library.path);
            }
        }
        context.document.components = std::move(set);
    }
}
